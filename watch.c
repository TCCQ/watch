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
  char action = 'a';
  char interpretFlags = 1;
  for (int i = 1; i < argc; i++) {//iterate over arguments

    if (interpretFlags && argv[i][0] == '-') {//flag
      if (argv[i][1] == '-') {// double -- flag, stop interpreting
        interpretFlags = 0; //false
      } else { //is a regular flag for this program
        switch (argv[i][1]) {//no flag combinations
          case 'h': //print help message
            printf("This is a hotkey script for linux that does not depend on X running. usage is watch [-k keycode] [-m modifier list] [-p] [-h] [--] command \n -k keycode specifies the keycode of the trigger key. defaults to 30 (a) if not specified \n -p removes the normal operation and instead prints the keycode of keyboard input, use to infom your usage of -k\n -h prints this message\n -m specify the modifier keys required to trigger command. c=control s=shift m=meta a=alt. case insensitive. defaults to ca if not specified. eg. AMS or ams or aMs is alt+meta+shift.\n -- is nessiary before the command if the command you want to run includes flags, or arguments starting with a dash. \n example:\n sudo watch -k 32 -m sca -- echo -n \"key activated!\" '>>' /example/path/to/file.txt \n This would append to the file. put pipes, etc in quotes to prevent the interactive shell from interpreting them. the command is run via sh, so don't get too fancy.\n be aware that there is a 256 character limit on the command, so put things in a script and run that if need be. \n you will need to change the path to the keyboard event in the source, at the top, called 'device' if you steal this. Best of luck!\n");
            return 0;
          case 'p': //print keycodes instead
            action = 'p';
            break;
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
      if (action == 'p') {
        printf("type: %i \n code: %i \n value:  %i \n \n",event.type, event.code, event.value);
        continue;     
      }

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
