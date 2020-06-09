# coding: utf8
import pyautogui as pg
import time

# COMMAND_COUNT = 0


class KeysTyping:

    @staticmethod
    def send_command(instr):
        """
        Here instr (abbr. for 'instruction') is a string representing the command to be sent to the robot's bluetooth
        module.

        return value:
        -1: wrong input of instr
        0: command sent successfully
        """

        # if not isinstance(instr, str):
        #     return -1
        #
        # global COMMAND_COUNT
        # if COMMAND_COUNT == 0:
        #     pg.hotkey('altleft', 'tab')
        #     time.sleep(0.5)
        #
        # pg.typewrite(instr)
        # pg.typewrite(['enter'])
        # COMMAND_COUNT += 1
        # time.sleep(1.5)

        #==============================================================================================

        pg.hotkey('altleft', 'tab')
        time.sleep(0.5)

        #依次输入指令
        for string in instr:
            pg.typewrite(string)
            pg.typewrite(['enter'])
            time.sleep(1.5)




# if __name__ == '__main__':
#     ret = send_command('w 500')
#     # time.sleep(1.5)
#     ret = send_command('x 500')