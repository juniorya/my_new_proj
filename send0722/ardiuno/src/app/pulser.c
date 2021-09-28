//
//
//

#include <config.h>

#include "app.h"
#include "../system/data_supplier.h"

#include "command_processor.h"


/*static*/ console_ctx cs;
static comp_ctx command_proc;
static data_supplier console_drain;

void app_initialize(void)
{
    data_supplier_init(&console_drain);
    comp_init(&command_proc, &console_drain);
    __console_init(&cs, comp_get_command, &command_proc);
    __console_attach_serial(&cs, __system_serial_get(APP_CONSOLE_SERIAL));

    data_supplier_attach_current_stream(&console_drain, __console_print_pb, &cs);
}

void app_cycle_tick(void)
{
/*    unsigned long ulNow = __micros();
    if (UL_POSITIVE(ulNow - ulWatchDogTO)) {
        proc_watchdog();
        ulWatchDogTO = ulNow + 500000; // 0.5 sec
    }
*/

     __console_tick(&cs);
    comp_tick(&command_proc);
    data_supplier_tick(&console_drain);
}
