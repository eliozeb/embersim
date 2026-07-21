/**
  ******************************************************************************
  * @file    main.c
  * @brief   Native entry point — gcc build only.
  *
  *          Compiled by the Makefile for the native (non-EmberSim) build.
  *          Not included in embersim.toml [build.sources] — EmberSim uses
  *          host_main.c which provides its own main() and calls app_init()
  *          and app_run() directly.
  ******************************************************************************
  */

#include "app.h"

int main(void)
{
    app_init();

    while (1) {
        app_run();
    }
}
