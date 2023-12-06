pub mod blockchain;
pub mod dir_iterator;
pub mod file;
pub mod filesystem;
pub mod internet;

use blockchain::get_call_params;

#[no_mangle]
pub unsafe extern "C" fn run() -> u32 {
    let params = get_call_params();
    assert_eq!(params, vec![]);

    return 0;
}