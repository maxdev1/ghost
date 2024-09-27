/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2015, Max Schlüssel <lokoxe@gmail.com>                     *
 *                                                                           *
 *  This program is free software: you can redistribute it and/or modify     *
 *  it under the terms of the GNU General Public License as published by     *
 *  the Free Software Foundation, either version 3 of the License, or        *
 *  (at your option) any later version.                                      *
 *                                                                           *
 *  This program is distributed in the hope that it will be useful,          *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *  GNU General Public License for more details.                             *
 *                                                                           *
 *  You should have received a copy of the GNU General Public License        *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
 *                                                                           *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef __LIBTERMINAL_TERMINAL__
#define __LIBTERMINAL_TERMINAL__

#include <ghost.h>

#define TERMINAL_STREAM_CONTROL_MAX_PARAMETERS 4

/**
 * ASCII codes, used for escaping
 */
#define G_TERMKEY_SUB 26
#define G_TERMKEY_ESC 27

/**
 * Extended key codes, derived from ncurses
 */
#define G_TERMKEY_MIN 0x101		  /* minimum extended key value */
#define G_TERMKEY_BREAK 0x101	  /* break key */
#define G_TERMKEY_DOWN 0x102	  /* down arrow */
#define G_TERMKEY_UP 0x103		  /* up arrow */
#define G_TERMKEY_LEFT 0x104	  /* left arrow */
#define G_TERMKEY_RIGHT 0x105	  /* right arrow*/
#define G_TERMKEY_HOME 0x106	  /* home key */
#define G_TERMKEY_BACKSPACE 0x107 /* Backspace */

#define G_TERMKEY_F0 0x108
#define G_TERMKEY_F(n) (G_TERMKEY_F0 + (n))

#define G_TERMKEY_DL 0x148	   /* Delete Line */
#define G_TERMKEY_IL 0x149	   /* Insert Line*/
#define G_TERMKEY_DC 0x14A	   /* Delete Character */
#define G_TERMKEY_IC 0x14B	   /* Insert Character */
#define G_TERMKEY_EIC 0x14C	   /* Exit Insert Char mode */
#define G_TERMKEY_CLEAR 0x14D  /* Clear screen */
#define G_TERMKEY_EOS 0x14E	   /* Clear to end of screen */
#define G_TERMKEY_EOL 0x14F	   /* Clear to end of line */
#define G_TERMKEY_SF 0x150	   /* Scroll one line forward */
#define G_TERMKEY_SR 0x151	   /* Scroll one line back */
#define G_TERMKEY_NPAGE 0x152  /* Next Page */
#define G_TERMKEY_PPAGE 0x153  /* Prev Page */
#define G_TERMKEY_STAB 0x154   /* Set Tab */
#define G_TERMKEY_CTAB 0x155   /* Clear Tab */
#define G_TERMKEY_CATAB 0x156  /* Clear All Tabs */
#define G_TERMKEY_ENTER 0x157  /* Enter or Send */
#define G_TERMKEY_SRESET 0x158 /* Soft Reset */
#define G_TERMKEY_RESET 0x159  /* Hard Reset */
#define G_TERMKEY_PRINT 0x15A  /* Print */
#define G_TERMKEY_LL 0x15B	   /* Home Down */

/*
 * "Keypad" keys arranged like this:
 *
 *  A1   up  A3
 * left  B2 right
 *  C1  down C3
 *
 */
#define G_TERMKEY_A1 0x15C /* Keypad upper left */
#define G_TERMKEY_A3 0x15D /* Keypad upper right */
#define G_TERMKEY_B2 0x15E /* Keypad centre key */
#define G_TERMKEY_C1 0x15F /* Keypad lower left */
#define G_TERMKEY_C3 0x160 /* Keypad lower right */

#define G_TERMKEY_BTAB 0x161	  /* Back Tab */
#define G_TERMKEY_BEG 0x162		  /* Begin key */
#define G_TERMKEY_CANCEL 0x163	  /* Cancel key */
#define G_TERMKEY_CLOSE 0x164	  /* Close Key */
#define G_TERMKEY_COMMAND 0x165	  /* Command Key */
#define G_TERMKEY_COPY 0x166	  /* Copy key */
#define G_TERMKEY_CREATE 0x167	  /* Create key */
#define G_TERMKEY_END 0x168		  /* End key */
#define G_TERMKEY_EXIT 0x169	  /* Exit key */
#define G_TERMKEY_FIND 0x16A	  /* Find key */
#define G_TERMKEY_HELP 0x16B	  /* Help key */
#define G_TERMKEY_MARK 0x16C	  /* Mark key */
#define G_TERMKEY_MESSAGE 0x16D	  /* Message key */
#define G_TERMKEY_MOVE 0x16E	  /* Move key */
#define G_TERMKEY_NEXT 0x16F	  /* Next Object key */
#define G_TERMKEY_OPEN 0x170	  /* Open key */
#define G_TERMKEY_OPTIONS 0x171	  /* Options key */
#define G_TERMKEY_PREVIOUS 0x172  /* Previous Object key */
#define G_TERMKEY_REDO 0x173	  /* Redo key */
#define G_TERMKEY_REFERENCE 0x174 /* Ref Key */
#define G_TERMKEY_REFRESH 0x175	  /* Refresh key */
#define G_TERMKEY_REPLACE 0x176	  /* Replace key */
#define G_TERMKEY_RESTART 0x177	  /* Restart key */
#define G_TERMKEY_RESUME 0x178	  /* Resume key */
#define G_TERMKEY_SAVE 0x179	  /* Save key */
#define G_TERMKEY_SBEG 0x17A	  /* Shift begin key */
#define G_TERMKEY_SCANCEL 0x17B	  /* Shift Cancel key */
#define G_TERMKEY_SCOMMAND 0x17C  /* Shift Command key */
#define G_TERMKEY_SCOPY 0x17D	  /* Shift Copy key */
#define G_TERMKEY_SCREATE 0x17E	  /* Shift Create key */
#define G_TERMKEY_SDC 0x17F		  /* Shift Delete Character */
#define G_TERMKEY_SDL 0x180		  /* Shift Delete Line */
#define G_TERMKEY_SELECT 0x181	  /* Select key */
#define G_TERMKEY_SEND 0x182	  /* Send key */
#define G_TERMKEY_SEOL 0x183	  /* Shift Clear Line key */
#define G_TERMKEY_SEXIT 0x184	  /* Shift Exit key */
#define G_TERMKEY_SFIND 0x185	  /* Shift Find key */
#define G_TERMKEY_SHELP 0x186	  /* Shift Help key */
#define G_TERMKEY_SHOME 0x187	  /* Shift Home key */
#define G_TERMKEY_SIC 0x188		  /* Shift Input key */
#define G_TERMKEY_SLEFT 0x189	  /* Shift Left Arrow key */
#define G_TERMKEY_SMESSAGE 0x18A  /* Shift Message key */
#define G_TERMKEY_SMOVE 0x18B	  /* Shift Move key */
#define G_TERMKEY_SNEXT 0x18C	  /* Shift Next key */
#define G_TERMKEY_SOPTIONS 0x18D  /* Shift Options key */
#define G_TERMKEY_SPREVIOUS 0x18E /* Shift Previous key */
#define G_TERMKEY_SPRINT 0x18F	  /* Shift Print key */
#define G_TERMKEY_SREDO 0x190	  /* Shift Redo key */
#define G_TERMKEY_SREPLACE 0x191  /* Shift Replace key */
#define G_TERMKEY_SRIGHT 0x192	  /* Shift Right Arrow key */
#define G_TERMKEY_SRSUME 0x193	  /* Shift Resume key */
#define G_TERMKEY_SSAVE 0x194	  /* Shift Save key */
#define G_TERMKEY_SSUSPEND 0x195  /* Shift Suspend key */
#define G_TERMKEY_SUNDO 0x196	  /* Shift Undo key */
#define G_TERMKEY_SUSPEND 0x197	  /* Suspend key */
#define G_TERMKEY_UNDO 0x198	  /* Undo key */
#define G_TERMKEY_MOUSE 0x199	  /* Mouse event has occurred */
#define G_TERMKEY_RESIZE 0x200	  /* Resize event has occurred */
#define G_TERMKEY_MAX 0x240		  /* maximum extended key value */

/**
 * Different terminal modes
 */
typedef int g_terminal_mode;
#define G_TERMINAL_MODE_DEFAULT ((g_terminal_mode) 0)
#define G_TERMINAL_MODE_RAW ((g_terminal_mode) 1)
#define G_TERMINAL_MODE_CBREAK ((g_terminal_mode) 2)

/**
 * Cursor position struct
 */
typedef struct _g_term_cursor_position
{
	_g_term_cursor_position(int x, int y) : x(x), y(y)
	{
	}
	_g_term_cursor_position() : x(0), y(0)
	{
	}
	int y;
	int x;
} g_term_cursor_position;

/**
 * Cursor position struct
 */
typedef struct _g_term_dimension
{
	_g_term_dimension(int w, int h) : w(w), h(h)
	{
	}
	_g_term_dimension() : w(0), h(0)
	{
	}
	int w;
	int h;
} g_term_dimension;

/**
 * Terminal client access class.
 */
class g_terminal
{
  private:
	static int readUnbuffered();
	static void bufferChar(int c);
	static void readAndBufferUntilESC();
	static int readEscapedParameters(int* parameters);

  public:
	static void setEcho(bool echo);
	static void setMode(g_terminal_mode mode);

	static int getChar();
	static void putChar(int c);

	static void setCursor(g_term_cursor_position position);
	static g_term_cursor_position getCursor();
	static void moveCursorUp(int n);
	static void moveCursorDown(int n);
	static void moveCursorForward(int n);
	static void moveCursorBack(int n);
    static void remove();

	static g_term_dimension getSize();
	static void setControlProcess(g_pid pid);

	static void clear();
	static void setScrollAreaToScreen();
	static void setScrollArea(int start, int end);
	static void scroll(int amount);
	static void setCursorVisible(bool visible);

	static void flush();
};

#endif
