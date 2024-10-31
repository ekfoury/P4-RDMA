#include "./registers.p4"

control LongFlowReverse(inout headers_t hdr, inout metadata_t meta) {
    #define SKETCH_REGISTER_ACTION(num) RegisterAction<bit<SKETCH_CELL_BIT_WIDTH>, bit<16>, bit<1>>(sketch##num) read_sketch_reverse##num = {\
        void apply(inout bit<SKETCH_CELL_BIT_WIDTH> register_data, out bit<1> result) { \
            if (register_data > THRESH) {\
                result = 1; \
            } else {\
                result = 0; \
            }\
        } \
    }; \
    action exec_read_reverse_sketch##num() { \
        meta.rev_value_sketch##num = read_sketch_reverse##num.execute(meta.reverse_index_sketch##num); \
    }

    SKETCH_REGISTER_ACTION(0)
    SKETCH_REGISTER_ACTION(1)
    SKETCH_REGISTER_ACTION(2)
    SKETCH_REGISTER_ACTION(3)
    
    apply {
        exec_read_reverse_sketch0();
        exec_read_reverse_sketch1();
        exec_read_reverse_sketch2();
        exec_read_reverse_sketch3();
        
    }

}