pub mod blockchain;
pub mod dir_iterator;
pub mod file;
pub mod filesystem;
pub mod internet;

use sdk::file::FileWriter;
use serial_test::serial;
use std::{
    cmp::min,
    io::{BufWriter, Write},
};

#[no_mangle]
pub unsafe extern "C" fn run() -> u32 {
    let test_case = "Hi wasmer.\nGood to see you.\n".as_bytes();
    let file = unsafe { FileWriter::new("../../test.txt".to_string()).unwrap() };
    let mut writer = BufWriter::new(file);
    let ret = writer.write(test_case).unwrap();
    // flush to ensure the buffer wrapped by the BufWriter is all written in the file
    writer.flush().unwrap(); // https://stackoverflow.com/questions/69819990/whats-the-difference-between-flush-and-sync-all#:~:text=So%20what%20is,%27s%20documentation).

    let file = unsafe { FileReader::new("../../test.txt".to_string()).unwrap() };
    let mut reader = BufReader::new(file);
    let mut big_buffer = Vec::new();
    reader.read_to_end(&mut big_buffer).unwrap();

    if test_case == big_buffer {
        return 1;
    }
    return 0;
}
