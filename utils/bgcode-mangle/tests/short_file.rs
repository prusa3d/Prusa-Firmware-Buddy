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

fn truncated_bgcode() -> Vec<u8> {
    let mut bytes = Vec::new();

    bytes.extend(b"GCDE");
    push_u32(&mut bytes, 1);
    push_u16(&mut bytes, 1);

    push_u16(&mut bytes, 1);
    push_u16(&mut bytes, 0);
    push_u32(&mut bytes, 0);

    bytes
}

fn partial_block_header_bgcode() -> Vec<u8> {
    let mut bytes = Vec::new();

    bytes.extend(b"GCDE");
    push_u32(&mut bytes, 1);
    push_u16(&mut bytes, 1);
    bytes.push(1);

    bytes
}

#[test]
fn truncated_block_reports_short_file() {
    let unique = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .unwrap()
        .as_nanos();
    let temp_dir = env::temp_dir().join(format!("bgcode-mangle-{}-{unique}", process::id()));
    fs::create_dir(&temp_dir).unwrap();

    let input = temp_dir.join("input.bgcode");
    let output = temp_dir.join("output.bgcode");
    fs::write(&input, truncated_bgcode()).unwrap();

    let output = Command::new(env!("CARGO_BIN_EXE_bgcode-mangle"))
        .arg(&input)
        .arg(&output)
        .output()
        .unwrap();
    assert!(!output.status.success());

    let stderr = String::from_utf8_lossy(&output.stderr);
    assert!(stderr.contains("Short file"));
    assert!(!stderr.contains("panicked"));

    fs::remove_dir_all(temp_dir).unwrap();
}

#[test]
fn partial_block_header_reports_error() {
    let unique = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .unwrap()
        .as_nanos();
    let temp_dir = env::temp_dir().join(format!("bgcode-mangle-{}-{unique}", process::id()));
    fs::create_dir(&temp_dir).unwrap();

    let input = temp_dir.join("input.bgcode");
    let output = temp_dir.join("output.bgcode");
    fs::write(&input, partial_block_header_bgcode()).unwrap();

    let output = Command::new(env!("CARGO_BIN_EXE_bgcode-mangle"))
        .arg(&input)
        .arg(&output)
        .output()
        .unwrap();
    assert!(!output.status.success());

    let stderr = String::from_utf8_lossy(&output.stderr);
    assert!(stderr.contains("Block type"));
    assert!(!stderr.contains("panicked"));

    fs::remove_dir_all(temp_dir).unwrap();
}
