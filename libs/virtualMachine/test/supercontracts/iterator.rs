pub mod blockchain;
pub mod dir_iterator;
pub mod file;
pub mod filesystem;
pub mod internet;

use crate::dir_iterator::DirIterator;
use crate::file::FileWriter;
// use crate::filesystem::*;
use std::io::{BufWriter, Write};

#[no_mangle]
pub unsafe extern "C" fn run() -> u32 {
    let test_case = "Hi wasmer.\nGood to see you.\n".as_bytes();
    let file = FileWriter::new("../../libs/test.txt").unwrap();
    let mut writer = BufWriter::new(file);
    writer.write(test_case).unwrap();
    writer.flush().unwrap();

    let mut iterator = DirIterator::new("../../libs", true);

    let expected = [
        "../../libs/crypto",
        "../../libs/CMakeLists.txt",
        "../../libs/logging",
        "../../libs/test.txt",
        "../../libs/utils",
        "../../libs/virtualMachine",
        "../../libs/messenger",
        "../../libs/internet",
    ];

    let mut i = 0;

    while let Some(path) = iterator.next() {
        if path != expected[i] {
            return 0;
        }
        if path == "../../libs/test.txt" {
            iterator.remove().unwrap();
        }
        i += 1;
    }

    return 1;
}
