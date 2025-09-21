/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PICOW_BT_EXAMPLE_COMMON_H
#define PICOW_BT_EXAMPLE_COMMON_H

// Add C++ compatibility guards
#ifdef __cplusplus
extern "C" {
#endif

/*
 * \brief Initialise BTstack example with cyw43
 *
 * \return 0 if ok
 */
int picow_bt_example_init(void);

/*
 * \brief Run the BTstack example
 *
 */
void picow_bt_example_main(void);


#ifdef __cplusplus
}
#endif

#endif // PICOW_BT_EXAMPLE_COMMON_H