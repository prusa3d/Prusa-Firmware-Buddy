use std::collections::HashSet;
use std::fs::File;
use std::io::{BufReader, BufWriter, ErrorKind, Read, Seek, Write};
use std::path::PathBuf;

use anyhow::{ensure, Context, Error};
use byteorder::{NativeEndian, ReadBytesExt, WriteBytesExt};
use clap::Parser;
use log::debug;
use num_enum::{IntoPrimitive, TryFromPrimitive};

#[derive(Copy, Clone, Debug, Default, IntoPrimitive, TryFromPrimitive)]
#[repr(u16)]
enum ChecksumType {
    #[default]
    None,
    Crc32,
}

#[derive(Copy, Clone, Debug, Default)]
struct FileHeader {
    magic: [u8; 4],
    version: u32,
    checksum_type: ChecksumType,
}

impl FileHeader {
    fn read<R: Read>(r: &mut R) -> Result<Self, Error> {
        let mut magic = [0; 4];
        r.read_exact(&mut magic).context("Magic")?;
        let version = r.read_u32::<NativeEndian>().context("Version")?;
        let checksum_type = r.read_u16::<NativeEndian>().context("Checksum")?;
        let checksum_type =
            ChecksumType::try_from(checksum_type).context("Invalid checksum value")?;
        Ok(Self {
            magic,
            version,
            checksum_type,
        })
    }

    fn write<W: Write>(&self, w: &mut W) -> Result<(), Error> {
        w.write_all(&self.magic).context("Magic")?;
        w.write_u32::<NativeEndian>(self.version)
            .context("Version")?;
        w.write_u16::<NativeEndian>(self.checksum_type.into())
            .context("Checksum type")?;
        Ok(())
    }

    fn check(&self) -> Result<(), Error> {
        ensure!(&self.magic == b"GCDE");
        ensure!(self.version == 1);

        Ok(())
    }

    fn crc_size(&self) -> u32 {
        match self.checksum_type {
            ChecksumType::None => 0,
            ChecksumType::Crc32 => 4,
        }
    }
}

#[derive(Copy, Clone, Debug, PartialEq, IntoPrimitive, TryFromPrimitive)]
#[repr(u16)]
enum BlockType {
    FileMetadata,
    Gcode,
    SlicerMetadata,
    PrinterMetadata,
    PrintMetadata,
    Thumbnail,
}

#[derive(Copy, Clone, Debug, PartialEq, IntoPrimitive, TryFromPrimitive)]
#[repr(u16)]
enum Compression {
    None,
    Deflate,
    Heatshrink11_4,
    Heatshrink12_4,
}

#[derive(Copy, Clone, Debug)]
struct BlockHeader {
    tp: BlockType,
    compression: Compression,
    uncompressed_size: u32,
    compressed_size: u32,
}

impl BlockHeader {
    fn read<R: Read>(r: &mut R) -> Result<Option<Self>, Error> {
        let block_type = match r.read_u16::<NativeEndian>() {
            Ok(t) => t,
            Err(e) if e.kind() == ErrorKind::UnexpectedEof => return Ok(None),
            Err(e) => Err(e).context("Block type")?,
        };
        let block_type = BlockType::try_from(block_type).context("Invalid block type")?;
        let compression = r.read_u16::<NativeEndian>().context("Compression")?;
        let compression = Compression::try_from(compression).context("Invalid compression")?;
        let uncompressed_size = r.read_u32::<NativeEndian>().context("Uncompressed size")?;
        let compressed_size = if compression == Compression::None {
            uncompressed_size
        } else {
            r.read_u32::<NativeEndian>().context("Compressed size")?
        };
        Ok(Some(Self {
            tp: block_type,
            compression,
            uncompressed_size,
            compressed_size,
        }))
    }

    fn write<W: Write>(&self, w: &mut W) -> Result<(), Error> {
        w.write_u16::<NativeEndian>(self.tp.into())
            .context("Block type")?;
        w.write_u16::<NativeEndian>(self.compression.into())
            .context("Compression")?;
        w.write_u32::<NativeEndian>(self.uncompressed_size)
            .context("Uncompressed size")?;
        if self.compression != Compression::None {
            w.write_u32::<NativeEndian>(self.compressed_size)
                .context("Compressed size")?;
        }
        Ok(())
    }

    /// The _real_ payload size.
    ///
    /// The design of bgcode stores the "inner" payload size. But it is preceded by some kind of
    /// encoding type and params, whose size depends on the type :facepalm:.
    ///
    /// It is also preceded by a checksum that is dependent on the _file header_.
    fn payload_size_with_params(&self) -> u32 {
        let encoding = match self.tp {
            BlockType::Thumbnail => 6,
            _ => 2,
        };
        encoding + self.compressed_size
    }
}

/// Kill some CRCs in the file.
///
/// For testing purposes.
#[derive(Parser, Debug)]
struct Args {
    /// The file to read.
    input: PathBuf,

    /// The file to write to.
    output: PathBuf,

    /// Kill these blocks.
    #[arg(short, long)]
    killblocks: Vec<usize>,
}

fn main() -> Result<(), Error> {
    env_logger::init();

    let args = Args::parse();

    let mut input = BufReader::new(
        File::open(&args.input)
            .with_context(|| format!("Opening input {}", args.input.display()))?,
    );

    let file_header = FileHeader::read(&mut input).context("File header")?;
    debug!("File header: {file_header:?}");

    let mut output = BufWriter::new(
        File::create(&args.output)
            .with_context(|| format!("Opening output {}", args.output.display()))?,
    );

    let out_header = file_header;

    out_header
        .write(&mut output)
        .context("Writing file header")?;

    file_header.check().context("Invalid file header")?;
    let checksum_size = file_header.crc_size() as usize;
    ensure!(checksum_size > 0, "No checksums present");

    let mut block = Vec::new();
    let mut idx = 0;
    let killblocks: HashSet<usize> = args.killblocks.into_iter().collect();
    while let Some(header) = BlockHeader::read(&mut input).context("Block header")? {
        debug!("Block header: {header:?}");
        block.clear();
        let size = header.payload_size_with_params() as usize + checksum_size;
        debug!("Block of size {size}");
        input
            .by_ref()
            .take(size as u64)
            .read_to_end(&mut block)
            .context("Reading block content")?;
        let l = block.len();
        let crc = &mut block[l - checksum_size..];
        let pos = output.stream_position().unwrap_or_default();
        println!(
            "Block {idx} @{pos}: {:?} ({}), CRC: {:?}, Size: {size}",
            header.tp, header.compressed_size, crc
        );
        if killblocks.contains(&idx) {
            println!("Killing CRC");
            for b in crc {
                *b += 1;
            }
        }
        ensure!(block.len() == size, "Short file");

        header.write(&mut output).context("Writing block header")?;
        output.write_all(&block).context("Writing block")?;
        idx += 1;
    }

    debug!("Done reading");

    Ok(())
}
