pub mod blockchain;
pub mod dir_iterator;
pub mod file;
pub mod filesystem;
pub mod internet;

#[no_mangle]
pub unsafe extern "C" fn run() -> u32 {
    return 1 + 1;
}
