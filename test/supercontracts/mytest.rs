mod blockchain;

use blockchain::*;

#[no_mangle]
pub extern "C" fn run() -> i32 {
    let assets = get_service_payments();
    // let mut total_amount = 0;
    // for item in assets {
    //     total_amount += item.amount;
    // }
    return 1;
}
