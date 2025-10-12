#pragma once
#include "input/keymanager.h"
#include "core/workspace/workspace.h"
#include "config.h"
#include <unistd.h>
#include <sys/types.h>
#include <iostream>
#include <sys/wait.h>

void spawn(){
    //huge thanks to catwm
    if(fork() == 0) {
        if(fork() == 0) {
            setsid();
            execlp("kitty","kitty");
        }
        exit(0);
    }
}

inline void register_keybindings(KeyManager &km) {
  km.add_binding(XCB_MOD_MASK_4, XK_Return, []() {
    spawn();
  });

  km.add_binding(XCB_MOD_MASK_4, XK_Right, []() {
    wom.next(); 
  });

  km.add_binding(XCB_MOD_MASK_4, XK_Left, []() {
    wom.previous(); 
  });
}
