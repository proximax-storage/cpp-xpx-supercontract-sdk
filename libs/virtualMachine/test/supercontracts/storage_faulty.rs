pub mod blockchain;
pub mod dir_iterator;
pub mod file;
pub mod filesystem;
pub mod internet;

use crate::file::FileWriter;

#[no_mangle]
pub unsafe extern "C" fn run() -> u32 {
    // let test_case = "Hi wasmer.\nGood to see you.\n".as_bytes();
    match FileWriter::new("../../test.txt") {
        Ok(_) => return 0,
        Err(_) => return 1,
    };
}
