#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/input.h>
#include <sys/time.h>

const char device[] = "/dev/input/by-path/platform-i8042-serio-0-event-kbd";
char command[256]; //nothing super long, can call script if need be.
int commandIndex;
char curMod, desMod; //0b0000 shift control meta alt
char keyCode; //NOT a = a, instead 30 = a (or whatever you key code is)

void flip(char* store, int index, char value) {
  if (value) {
    *store = *store | (0x01 << index);
  } else {
    *store = *store & (~(0x01 << index));
  }
}

int main (int argc, char* argv[]) {
  curMod = 0;
  desMod = 0x05;
  keyCode = 30; //initiate
  commandIndex = 0;
  char action = 'p';
  char interpretFlags = 1;
  for (int i = 1; i < argc; i++) {//iterate over arguments

    if (interpretFlags && argv[i][0] == '-') {//flag
      if (argv[i][1] == '-') {// double -- flag, stop interpreting
        interpretFlags = 0; //false
      } else { //is a regular flag for this program
        switch (argv[i][1]) {//no flag combinations
          case 'k': //set keycodes
            if (i+1 >= argc) {
              printf("Didn't specify a modifier combination, not enough arguments\n");
              return 1;
            }
            keyCode = atoi(argv[i+1]);
            i++;
            break;
          case 'm': //set modifiers
            if (i+1 >= argc) {
              printf("Didn't specify a modifier combination, not enough arguments\n");
              return 1;
            }
            desMod = 0x00;
            int j = 0;
            while (argv[i+1][j] != '\x00') {
              switch (argv[i+1][j]) {
                case 'S':
                case 's': //case insensitive
                  desMod = desMod | 0x08;
                  break;
                case 'C':
                case 'c':
                  desMod = desMod | 0x04;
                  break;
                case 'M':
                case 'm':
                  desMod = desMod | 0x02;
                  break;
                case 'A':
                case 'a':
                  desMod = desMod | 0x01;
                  break;
              }
              j++;
            }
            i++;
            break;
          default:
            action = argv[i][1];
            break;
        }
      }
    } else { //nonflag argument (or anything past --)
      int len = strlen(argv[i]);
      memcpy(command+commandIndex, argv[i], len);
      commandIndex += len + 1;
      command[commandIndex-1] = ' '; //seperate arguments
    }
  }
  command[commandIndex] = '\x00';
  //printf("%s\n", command);
  //return 0;

  FILE* kbd = fopen(device, "r");

  if (kbd == NULL) {
    printf("error opening /dev/input/... for keyboard\n");
    return 1;
  }
  

  struct input_event event;
  const int readsize = sizeof(struct input_event);
  while (1) {
    fread(&event, readsize, 1, kbd);
    if (event.type == EV_KEY) {
//      if (action = 'p') {
//        printf("type: %i \n code: %i \n value:  %i \n \n",event.type, event.code, event.value);
//        continue;     
//      }

      switch (event.code) {
        case 42: //shift
          flip(&curMod, 3, event.value); 
          break;
        case 29: //control
          flip(&curMod, 2, event.value);
          break;
        case 125: //meta
          flip(&curMod, 1, event.value);
          break;
        case 56: //alt
          flip(&curMod, 0, event.value);
        default: //catch
          //printf("default, %x \n", curMod);
          if (event.value == 1 && event.code == keyCode && curMod == desMod) {
            //printf("%s\n", command);
            system(command);
          }
      }
    }
  }
  return 0;
}
