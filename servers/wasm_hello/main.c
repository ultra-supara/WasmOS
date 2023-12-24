#include <libs/common/print.h>
#include "wasm_export.h"

#define STACK_SIZE  (4096U)
#define HEAP_SIZE   (4096U)

// defined in main.S
extern unsigned char wasm_start[];
extern int wasm_len[];

// host function
void info(wasm_exec_env_t exec_env, const char *str) {
    INFO("%s", str);
}

int main(void) {
    // init runtime
    static char global_heap_buf[512 * 1024];
    char error_buf[128];

    // register host functon
    static NativeSymbol native_symbols[] = {
        EXPORT_WASM_API_WITH_SIG(info, "($)")
    };

    RuntimeInitArgs init_args ={
        .mem_alloc_type = Alloc_With_Pool,
        .mem_alloc_option.pool.heap_buf = global_heap_buf,
        .mem_alloc_option.pool.heap_size = sizeof(global_heap_buf),
        .n_native_symbols = sizeof(native_symbols) / sizeof(NativeSymbol),
        .native_module_name = "env",
        .native_symbols = native_symbols
    };

    if(!wasm_runtime_full_init(&init_args)) {
        ERROR("init runtime environment failed");
        return ERR_ABORTED;
    }

    // decode
    wasm_module_t module = NULL;
    module = wasm_runtime_load(
        wasm_start, 
        wasm_len[0], 
        error_buf, 
        sizeof(error_buf)
    );
    if (!module) {
        ERROR("load wasm module failed");
        goto fail_1;
    }

    // instantiate
    wasm_module_inst_t moduleinst = NULL;
    moduleinst = wasm_runtime_instantiate(
        module,
        STACK_SIZE,
        HEAP_SIZE,
        error_buf,
        sizeof(error_buf)
    );
    if(!moduleinst) {
        ERROR("instantiation failed");
        goto fail_2;
    }

    char *argv[] = {NULL};
    if (!wasm_application_execute_main(moduleinst, 0, argv)) {
        ERROR("main function failed");
    }
    INFO("ret = %d", *(int *)argv);

    // destroy the module instance  
    wasm_runtime_deinstantiate(moduleinst);
    
    fail_2:
        // unload the module
        wasm_runtime_unload(module);
    fail_1:
        // destroy runtime environment
        wasm_runtime_destroy();
        
        return 0;
}