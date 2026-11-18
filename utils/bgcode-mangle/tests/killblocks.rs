use std::{
    env, fs,
    process::{self, Command},
    time::{SystemTime, UNIX_EPOCH},
};

fn push_u16(buf: &mut Vec<u8>, value: u16) {
    buf.extend(value.to_ne_bytes());
}

fn push_u32(buf: &mut Vec<u8>, value: u32) {
    buf.extend(value.to_ne_bytes());
}

fn one_block_bgcode(crc: [u8; 4]) -> Vec<u8> {
    let mut bytes = Vec::new();

    bytes.extend(b"GCDE");
    push_u32(&mut bytes, 1);
    push_u16(&mut bytes, 1);

    push_u16(&mut bytes, 1);
    push_u16(&mut bytes, 0);
    push_u32(&mut bytes, 0);
    bytes.extend([0, 0]);
    bytes.extend(crc);

    bytes
}

#[test]
fn killing_crc_wraps_ff_bytes() {
    let unique = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .unwrap()
        .as_nanos();
    let temp_dir = env::temp_dir().join(format!("bgcode-mangle-{}-{unique}", process::id()));
    fs::create_dir(&temp_dir).unwrap();

    let input = temp_dir.join("input.bgcode");
    let output = temp_dir.join("output.bgcode");
    fs::write(&input, one_block_bgcode([0xff, 0x00, 0xfe, 0x7f])).unwrap();

    let status = Command::new(env!("CARGO_BIN_EXE_bgcode-mangle"))
        .arg(&input)
        .arg(&output)
        .arg("-k")
        .arg("0")
        .status()
        .unwrap();
    assert!(status.success());

    let output_bytes = fs::read(&output).unwrap();
    assert_eq!(
        &output_bytes[output_bytes.len() - 4..],
        &[0x00, 0x01, 0xff, 0x80]
    );

    fs::remove_dir_all(temp_dir).unwrap();
}
