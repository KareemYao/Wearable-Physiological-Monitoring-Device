#ifndef AT_SLE_MAC_H
#define AT_SLE_MAC_H

#include "AT_Header.h"

// check GCC/Clang compilers' predefined __BYTE_ORDER__ Macro
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    #define IS_LITTLE_ENDIAN 1
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    #define IS_LITTLE_ENDIAN 0
#else
    // ARMCC V5 on Cortex-M is always little-endian
    #define IS_LITTLE_ENDIAN 1
#endif

// Compile-time assert compatible with ARMCC V5 (no C11 _Static_assert)
#define CT_ASSERT(expr, msg) typedef char ct_assert_##msg[(expr) ? 1 : -1]

#if defined(__CC_ARM)
    // ARMCC V5: use #pragma pack instead of __attribute__((packed))
    #pragma pack(push, 1)
    typedef union {
        struct {
            uint8_t High : 4;
            uint8_t Low  : 4;
        }Hex_Digit ;
        uint8_t Raw;
    }Hex_Object;

    typedef struct {
        Hex_Object byte[6];
    }Mac_Address;
    #pragma pack(pop)
#else
    typedef union __attribute__((packed)) {
        struct __attribute__((packed)) { // Small-Endian
            uint8_t High : 4;
            uint8_t Low  : 4;
        }Hex_Digit ;
        uint8_t Raw;
    }Hex_Object;

    typedef struct __attribute__((packed)) {
        Hex_Object byte[6];
    }Mac_Address;
#endif


// Expected:
//      In Little-Endian mode, Low(0xB) occupies the lower 4 bits and High(0xA)
//      the upper 4 bits, resulting in 0xAB.
//
// If compilation fails on your STM32, it indicates the compiler implements bit-fields
// as Big-Endian for this architecture.
//
// In that case, swap the declaration order of 'Low' and 'High' above.
CT_ASSERT(IS_LITTLE_ENDIAN, endianness_check);


// Additional check: Ensure the total struct size is strictly 6 bytes (prevents compiler tail padding).
CT_ASSERT(sizeof(Mac_Address) == 6, mac_address_size_check);


Mac_Address 
Get_SLE_Mac_Address(
    UART_HandleTypeDef* AT_Channel,
    uint8_t* r_buffer
);


HAL_StatusTypeDef 
Set_SLE_Mac_Address(
    UART_HandleTypeDef* AT_Channel,
    uint8_t* r_buffer
);


#endif // AT_SLE_MAC_H
