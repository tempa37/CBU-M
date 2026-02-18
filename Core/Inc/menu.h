#ifndef _MICRO_MENU_H_
#define _MICRO_MENU_H_

#include "stdint.h"

typedef void (*SelectCallback_t) (void);
typedef void (*EnterCallback_t) (uint8_t dir);

typedef struct Menu_Item {
  struct Menu_Item *Next; /**< Pointer to the next menu item of this menu item */
  struct Menu_Item *Previous; /**< Pointer to the previous menu item of this menu item */
  struct Menu_Item *Parent; /**< Pointer to the parent menu item of this menu item */
  struct Menu_Item *Child; /**< Pointer to the child menu item of this menu item */
  void (*SelectCallback)(void); /**< Pointer to the optional menu-specific select callback of this menu item */
  void (*EnterCallback)(uint8_t dir); /**< Pointer to the optional menu-specific enter callback of this menu item */
  const char* Text; /**< Menu item text to pass to the menu display callback function */
  const uint8_t id;
} Menu_Item_t;

#define MENU_ITEM(Name, Next, Previous, Parent, Child, SelectFunc, EnterFunc, Text, id) \
extern Menu_Item_t Next; \
extern Menu_Item_t Previous; \
extern Menu_Item_t Parent; \
extern Menu_Item_t Child; \
Menu_Item_t Name = {&Next, &Previous, &Parent, &Child, SelectFunc, EnterFunc, Text, id}

extern Menu_Item_t* CurrentMenuItem;
extern Menu_Item_t Menu_1;
extern Menu_Item_t Menu_4;
extern Menu_Item_t Menu_5;
extern Menu_Item_t Menu_7;
extern Menu_Item_t Me_1_1_2;
extern Menu_Item_t Me_1_2_2;

void Menu_EnterCurrentItem(uint8_t dir);
void Menu_Navigate(Menu_Item_t* const NewMenu);
void delay_save_callback(void *argument);

#endif /* _MICRO_MENU_H_ */