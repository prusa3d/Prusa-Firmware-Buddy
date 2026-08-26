#!/usr/bin/env python3
import pybgcode as bg

def dump_info(fn):
    fp = bg.open(fn, 'r')
    file_header = bg.FileHeader()
    file_header.read(fp)

    block_header = bg.BlockHeader()


    res = bg.read_next_block_header(fp, file_header, block_header)
    TYPES = dict(enumerate([
        'FileMetadata',
        'GCode',
        'SlicerMetadata',
        'PrinterMetadata',
        'PrintMetadata',
        'Thumbnail'
    ]))
    while res == bg.EResult.Success:
        print(f'Block type: {TYPES.get(block_header.type, block_header.type)}  size {block_header.compressed_size} / {block_header.uncompressed_size}')
        cls = None
        if block_header.type == bg.EBlockType.FileMetadata.value:
            cls = bg.FileMetadataBlock
        elif block_header.type == bg.EBlockType.PrinterMetadata.value:
            cls = bg.PrinterMetadataBlock
        elif block_header.type == bg.EBlockType.PrintMetadata.value:
            cls = bg.PrintMetadataBlock
        # elif block_header.type == bg.EBlockType.Thumbnail.value:
        #     cls = bg.ThumbnailBlock
        elif block_header.type == bg.EBlockType.SlicerMetadata.value:
            tp = bg.peek_slicer_metadata_block(fp, block_header)
            if tp == bg.EPeekSlicerMetadataResult.SlicerMetadataFound:
                cls = bg.SlicerMetadataBlock
            elif tp == bg.EPeekSlicerMetadataResult.Slicer3MetadataFound:
                cls = bg.Slicer3MetadataBlock
        if cls is not None:
            metadata = cls()
            metadata.read_data(fp, file_header, block_header)
            for k, v in metadata.raw_data:
                w = v if len(v) < 120 else f'... ({len(v)} bytes)'
                print(f'    {k}: {w}')
        else:
            bg.skip_block_content(fp, file_header, block_header)
        res = bg.read_next_block_header(fp, file_header, block_header)



if __name__ == '__main__':
    import sys
    for fn in sys.argv[1:]:
        dump_info(fn)
