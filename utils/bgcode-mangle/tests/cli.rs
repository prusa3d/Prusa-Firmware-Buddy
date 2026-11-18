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

fn one_block_bgcode() -> Vec<u8> {
    let mut bytes = Vec::new();

    bytes.extend(b"GCDE");
    push_u32(&mut bytes, 1);
    push_u16(&mut bytes, 1);

    push_u16(&mut bytes, 1);
    push_u16(&mut bytes, 0);
    push_u32(&mut bytes, 0);
    bytes.extend([0, 0]);
    push_u32(&mut bytes, 0);

    bytes
}

#[test]
fn insert_at_block_count_appends_after_last_block() {
    let unique = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .unwrap()
        .as_nanos();
    let temp_dir = env::temp_dir().join(format!("bgcode-mangle-{}-{unique}", process::id()));
    fs::create_dir(&temp_dir).unwrap();

    let input = temp_dir.join("input.bgcode");
    let output = temp_dir.join("output.bgcode");
    let input_bytes = one_block_bgcode();
    fs::write(&input, &input_bytes).unwrap();

    let status = Command::new(env!("CARGO_BIN_EXE_bgcode-mangle"))
        .arg(&input)
        .arg(&output)
        .arg("-i")
        .arg("1")
        .status()
        .unwrap();
    assert!(status.success());

    let output_bytes = fs::read(&output).unwrap();
    assert_eq!(&output_bytes[..input_bytes.len()], input_bytes.as_slice());
    assert_eq!(output_bytes.len(), input_bytes.len() + 14);
    assert_eq!(
        &output_bytes[input_bytes.len()..input_bytes.len() + 2],
        &3u16.to_ne_bytes()
    );

    fs::remove_dir_all(temp_dir).unwrap();
}
