/*           HyperText Browser for Dumb Terminals                      WWW.c
**           ====================================                      14-8-91
**  
**  Authors:
**	NP  Nicola Pellow  Tech.Student CERN 1990, 91
**
** History:
**   4 Dec 90	Written from scratch (NP)
**  17 Dec 90   Restructuring of program into functions 
**  16 Jan 91   Included handling of a glossary 
**   7 Feb 91   Completed a working version to be linked with Bernds Code.
**              Still plenty of things to do to improve it
**  11 Feb 91   Tim wrote the code so that it could be linked with Bernds Code.
**              This involves using file numbers rather than file pointers.    
**  18 Mar 91   History mechanism completed so the user have a  record of all
**              the previous documents they have viewed.
**   6 Apr 91   Input file is read into a buffer, so that the server is freed
**              as quickly as possible. Program now uses the external modules
**              HTBufferFile.c and HTBufferFile.h.
**   9 Apr 91   Executable version of WWW available on PRIAM, to be tested
**              by a few users, for feedback on bugs and improvements.
*/


/* Default Addresses */
/* ================= */

#define LOGICAL_DEFAULT "WWW_HOME"  /* Define me to be your home page */

#ifndef vms
#define DEFAULT_ADDRESS "/usr/local/lib/WWW/default.html" /* path only */ 
#else
#define DEFAULT_ADDRESS \
 "file://cernvax/userd/tbl/hypertext/WWW/LineMode/Defaults/default.html"
#endif


/* Check Statements */
/* ================ */

#define NAME_CHECK 0
#define FUNCTION_CHECK 0 


/* Include Files */
/* ============= */

#ifdef SHORT_NAMES 
#include "HTShort.h"
#endif

#include "HTUtils.h"		    /* WWW general purpose macros */
#include "tcp.h"		    /* TCP/IP and file access */
#include "WWW.h"		    /* WWW project constants etc */
#include "HTTCP.h"		    /* TCP/IP utilities */

#include "HTParse.h"                /* WWW address manipulation */
#include "HTAccess.h"               /* WWW document access network code */
#include "HTBufferFile.h"           /* Buffering of unformatted data */
#include "HTFormatText.h"           /* Buffering of formatted text */

/* Define Statements */
/* ================= */

#define SCREEN_SIZE HTScreenSize      /* Number of lines to the screen */

#ifndef SCREEN_WIDTH      
#ifdef VM                               /* Number of characters across the ..*/
#define SCREEN_WIDTH 78                 /* width of the screen */ 
#else
#define SCREEN_WIDTH 80
#endif
#endif

#ifdef VM                             /* Needed to flush out the prompt line..*/
#define NEWLINE_PROMPT                  /* before input */
#endif

#define	ADDRESS_LENGTH 256	      /*  Maximum address length of node */
#define TITLE_LENGTH 256              /*  Maximum length of a title */

#ifndef BOOL
#define BOOL unsigned char            /* Return codes for functions */
#endif
#ifndef NO
#define NO  (BOOL)0               
#define YES (BOOL)1               
#endif

#ifndef EOF
#define EOF (-1)                      /* End of file character defined as -1 */
#endif

#define WHITE_SPACE(c) ((c==' ')||(c=='\t')||(c=='\n')||(c=='\r'))
                                     /* Name for any kind of white space */ 

int   WWW_TraceFlag = 0;             /* Off unless -v option given */

#define SPACES(n) (&space_string[SCREEN_WIDTH - (n)])   /* String containing 
                                                           blank spaces only */

#define RESPONSE_LENGTH 255          /* Length of string for users response */
#ifndef PRIVATE
#define PRIVATE static	             /* Restrict visibility to this file */
#endif

/*	Public Variables
**	================
*/

PUBLIC int HTScreenSize = 24;		/* By default */

PUBLIC  BOOL interactive = YES;         /* User interaction i.e. shows prompts
                                           etc */
PUBLIC  BOOL end_of_file = NO;	        /* Is it the end of the file? */

PUBLIC  BOOL bottom_of_formatted_buffer = NO;   /* Is the formatted buffer
                                                   empty ? */ 


/* Structure for Determining the Paragraph Layout */
/* ============================================== */

typedef    struct para_struct {

/*
** Two left indents used for glossary's and ordered lists.
*/
	   int                     left_indent1;                       
	   int                     left_indent2;                       

	   int                     right_margin_size;

	   int                     alignment;                 /* 0=left   */
							      /* 1=right  */
							      /* 2=centre */
/*     
** Two capitalize states one for each of the left indents.
*/
	   BOOL                    capitalize1;                  /* 0=no  */
	   BOOL                    capitalize2;                  /* 1=yes */

	   BOOL                    double_spacing;               /* 0=no  */
								 /* 1=yes */

	   int                     lines_before;                /* 0=none */
								/* 1=one  */
	   int                     lines_after;                 /* 2=two  */
								/* etc..  */
               
    } para_style ;

/*  Examples of Style Sheets */
/*  ======================== */

/* Assuming the screeen is 80 characters across */

#ifdef TEST_STYLE_SHEET

/*                                          Indents  Al  Capi Dbl   Lines
**					   L1  L2  R      1  2 spac B4 After
*/
PRIVATE para_style     normal_style     = { 3,  3,  5, 0, 0, 0, 0,  0, 0};

PRIVATE para_style     list_style       = { 7, 10,  5, 0, 0, 0, 0,  1, 1};

PRIVATE para_style     glossary_style   = { 7, 27,  5, 0, 0, 0, 0,  1, 1};

PRIVATE para_style     mono_style       = { 0,  0,  0, 0, 0, 0, 0,  0, 0};

PRIVATE para_style     address_style    = { 4,  4,  5, 1, 0, 0, 0,  0, 0};

PRIVATE para_style     heading_style[7] = {

/* TITLE */				  { 0,  0,  5, 1, 0, 0, 0,  0, 1},


/* H1 */                                  { 0,  0,  0, 2, 1, 1, 0,  1, 1},

/* H2 */                                  { 0,  0,  0, 0, 1, 1, 0,  0, 0},

/* H3 */                                  { 2,  2,  0, 0, 0, 0, 0,  0, 0},

/* H4 */                                  { 4,  4,  0, 0, 0, 0, 0,  0, 0},

/* H5 */                                  { 6,  6,  0, 0, 0, 0, 0,  0, 0},

/* H6 */                                  { 8,  8,  0, 0, 0, 0, 0,  0, 0},

} ; /* end of heading styles */

#else
#ifdef OLD_STYLE_SHEET

/*                                          Indents   Al  Capi Dbl   Lines
**					   L1  L2  R      1  2 spac B4 After
*/
PRIVATE para_style     normal_style     = { 5,  4,  5, 0, 0, 0, 0,  0, 0};

PRIVATE para_style     list_style       = {10, 10,  5, 0, 0, 0, 0,  1, 1};

PRIVATE para_style     glossary_style   = {10, 30,  5, 0, 0, 0, 0,  1, 1};

PRIVATE para_style     mono_style       = { 0,  0,  5, 0, 0, 0, 0,  1, 1};

PRIVATE para_style     address_style    = { 5,  4,  5, 1, 0, 0, 0,  0, 0};

PRIVATE para_style     heading_style[7] = {

/* TITLE */				  { 10, 10,  0, 2, 1, 1, 1,  1, 1},

/* H1 */                                  {  2,  2,  5, 0, 1, 1, 1,  1, 1},

/* H2 */			          {  4,  4,  5, 0, 1, 1, 0,  1, 0},

/* H3 */			          {  4,  4,  5, 0, 0, 0, 0,  1, 0},

/* H4 */			          {  4,  4,  5, 0, 0, 0, 0,  1, 0},

/* H5 */			          {  4,  4,  5, 0, 0, 0, 0,  1, 0},

/* H6 */			          {  4,  4,  5, 0, 0, 0, 0,  1, 0},

}; /* end of heading styles */

#else

/*                                          Indents   Al  Capi Dbl   Lines
**                                         L1  L2  R      1  2 spac B4 After
*/
PRIVATE para_style     normal_style     = { 3,  3,  0, 0, 0, 0, 0,  0, 0};

PRIVATE para_style     list_style       = { 7, 10,  0, 0, 0, 0, 0,  1, 1};

PRIVATE para_style     glossary_style   = { 7, 27,  0, 0, 0, 0, 0,  1, 1};

PRIVATE para_style     mono_style       = { 0,  0,  0,  0, 0, 0, 0,  0, 0};

PRIVATE para_style     address_style    = { 4,  4,  0, 1, 0, 0, 0,  0, 0};

PRIVATE para_style     heading_style[7] = {

/* TITLE */                               { 0,  0,  0, 1, 0, 0, 0,  0, 1},

/* H1 */                                  { 0,  0,  0, 2, 1, 1, 0,  1, 1},

/* H2 */                                  { 0,  0,  0, 0, 1, 1, 0,  0, 0},

/* H3 */                                  { 2,  2,  0, 0, 0, 0, 0,  0, 0},

/* H4 */                                  { 4,  4,  0, 0, 0, 0, 0,  0, 0},

/* H5 */                                  { 6,  6,  0, 0, 0, 0, 0,  0, 0},

/* H6 */                                  { 8,  8,  0, 0, 0, 0, 0,  0, 0},

} ; /* end of heading styles */

#endif
#endif
 
/* State Machine */
/* ============= */

enum state_enum {S_text           , S_tag_start     , S_title,
                 S_tag_h          , S_tag_h_p       , S_tag_h_num,
                 S_tag_p          , S_tag_a         , S_tag_l,
                 S_tag_li         , S_tag_d         , S_tag_end,
                 S_tag_end_h      , S_tag_end_h_num , S_tag_end_a,
                 S_high_phrase    , S_hp1           , S_hp2,
                 S_high_phrase1   , S_high_phrase2  , S_anchor,
                 S_href           , S_aname         , S_plaintext,
                 S_xmp            , S_tag_in_xmp    ,
                 S_check_tag_in_xmp,
                 S_check_tag_in_xmp_x,
                 S_check_tag_in_xmp_xm,
                 S_check_tag_in_xmp_xmp,
                 S_false_xmp_tag,
                 S_junk_tag };

PRIVATE  enum state_enum  state;


/* Structure for Accumulating HyperText References */
/* =============================================== */

typedef struct anchor_struct {

       char                  *address;         /* Pointer to the address
                                                  of an anchor */
       } anchor; 


/* Arrays for storing the HyperText References */ 
PRIVATE anchor     href[ADDRESS_LENGTH];              /* Stores  HREF's */
PRIVATE anchor     name[ADDRESS_LENGTH];              /* Stores NAME's */

PRIVATE int  href_count;                       /* Counts the number of HREF's */
PRIVATE int  name_count;                       /* Counts the number of NAME's */

PRIVATE char chosen_reference[ADDRESS_LENGTH]; /* Address of requested node*/

PRIVATE char *current_address = 0;             /* Address of the current node */


/* Structure for History Mechanism */
/* =============================== */

typedef struct node_history {

               char                 *address;   /* Pointer to all the address's
                                                   of nodes visited */
               char                 *title;
               struct node_history  *next_node;
               struct node_history  *prev_node;

               } history;

PRIVATE history  * home = 0;			/* Root of history list */


/* PRIVATE (static global) Variables */
/* ========================= */

PRIVATE para_style * current_style;       /* Present layout style */

PRIVATE  char      * full_address=0;        /* Full path name of current node */ 

PRIVATE  BOOL       display_anchors = YES; /* Flag to say if position of
                                              anchors should be shown in text */
PRIVATE  BOOL       first_page = YES;    /* Flag to show that it is the first
                                            page of file to be displayed
                                            and input buffer can be filled */
PRIVATE  char       character;           /* Present character that is being
                                            parsed */
PRIVATE  char       line[SCREEN_WIDTH];  /* Stores a line of characters 
                                            to be output on the screen */
PRIVATE  int        position_in_line;    /* Position of a character
                                            contained within line[] */
PRIVATE  char       space_string[SCREEN_WIDTH + 1];  /* String filled with 
                                                        blank spaces, used for 
                                                        for producing margins */
PRIVATE  BOOL       title_found =NO;     /* Flag used to indicate that a title 
                                            has been found in the text */
PRIVATE  BOOL       both_indent = NO;    /* Flag to show that both of the 
                                            left indents are being used
                                            within the line being read into
                                            the character string 'line[]' */
PRIVATE  BOOL       begin_indent_change = NO;   /* Flag to show that the next 
                                                   characters read from the 
                                                   input buffer apply to the
                                                   new indent change */
PRIVATE  int        line_length;         /* Stores the number of spaces the
                                            text can occupy in a line */
PRIVATE  int        left_indent;         /* Is the current setting of the left
                                            indent */
PRIVATE  BOOL       capitalize;          /* Is the current setting of
                                            capitalize depending on the left
                                            indent */
PRIVATE  int        line_count;          /* Counts the number of lines output
                                            to produce a page at a time */
PRIVATE  BOOL	    is_index;            /* Is this node a valid index? */

PRIVATE  int        blank_lines;         /* Stores the number of blank lines 
                                            that should be left after a change
                                            of style or new paragraph etc. */
PRIVATE  char       choice[RESPONSE_LENGTH];    /* Stores the users response
                                                   to prompts */
PRIVATE  int        node_count = 0;      /* Counts the number of nodes
                                            accessed */

/* Declaration of Functions */
/* ======================== */

/* ( Possible arrangements of functions into smaller modules ) */
/* ----------------------------------------------------------- */

#ifdef __STDC__

/* WWW.c */

void Format_Lines(void);


/* Node_history_and_input */

BOOL Open_File(const char *file_name);
void Make_History_Links(void);
void Move_Pointer_Open_File_Link(int recall_node_number); 
void Backup_Pointer_Open_File(int recall_node_number);      
void History_List(void);   /* MAYBE  HT_options */            
void History_Back(void);   /* MAYBE  HT_options */            


/* HT_text_handling */

void Treat_One_Character(void); 
BOOL Check(char *s);


/* HT_output_text */

void Character_Build(char character);             
void Line_Wrap_and_Output(void);
void Line_to_Buffer(void);
void End_of_Unformatted_Text(void);
void Excess_Line_Characters(void);
void Start_Style(para_style *new_style);
void End_Style(void);
void Paragraph_End(void);
void New_Line(void);
void Change_Indent(int indent);          


/* HT_options */

void Selection_Prompt(void);
void User_Selection(void);
BOOL Check_User_Input(char *s);          
void Error_Selection(void);

void Help_Menu(void);
void Reference_List(void);
void Select_Reference(int ref_num);
void Search(char * keywords);        

#else

/* WWW.c */

void Format_Lines();


/* Node_history_and_input */

BOOL Open_File();
void Make_History_Links();
void Move_Pointer_Open_File_Link(); 
void Backup_Pointer_Open_File();      
void History_List();   /* MAYBE  HT_options */            
void History_Back();   /* MAYBE  HT_options */            


/* HT_text_handling */

void Treat_One_Character(); 
BOOL Check();


/* HT_output_text */

void Character_Build();             
void Line_Wrap_and_Output();
void Line_to_Buffer();
void End_of_Unformatted_Text();
void Excess_Line_Characters();
void Start_Style();
void End_Style();
void Paragraph_End();
void New_Line();
void Change_Indent();          


/* HT_options */

void Selection_Prompt();
void User_Selection();
BOOL Check_User_Input();          
void Error_Selection();
void Help_Menu();
void Reference_List();
void Select_Reference();
void Search();        

#endif

/*----------------------------------------------------------------------------*/
/* ===================== */
/* START OF MAIN PROGRAM */
/* ===================== */

#ifdef __STDC__
int main(int argc, char *argv[])
#else
int main(argc, argv)
    int   argc;
    char  *argv[];
#endif

{
    int  arg;			/* Argument number as we scan */
    BOOL first_keyword = YES;
    BOOL arguement_found = NO;
    char default_default[100];	/* Parse home]relative to this */

    strcpy(default_default, "file://");
    strcat(default_default, HTHostName());  /* Eg file://cernvax.cern.ch */

/*	Check for command line options
*       ------------------------------
*/

    for (arg=1; arg<argc ; arg++) {
        if (*argv[arg]=='-'){

	    if (0==strcmp(argv[arg], "-v")){

                WWW_TraceFlag = 1;		/* Verify: Turns on trace */

	    } else if (argv[arg][1] == 'p') {	/* Page size */

		if (sscanf(argv[arg]+2, "%d", &HTScreenSize) <1)  /* fail */
		    HTScreenSize = 999999;	/* Turns off paging */

	    } else if (0==strcmp(argv[arg], "-n")) {   /* Non-interactive */

		interactive = NO;	             /* Turns off interaction */

	    } else if (0==strcmp(argv[arg], "-a")) { /* No anchors */

		display_anchors = NO;	         /* Turns off anchor display */
	    }

        } else {

/*	    Check for main argument
            -----------------------
*/
	    if (!arguement_found) {
							  /* TBL */
		char * ref = HTParse(argv[arg], default_default, 
		    PARSE_ACCESS|PARSE_HOST|PARSE_PATH|PARSE_PUNCTUATION);
		strcpy(chosen_reference,ref);
		free(ref);
                arguement_found = YES;

	    } else { 

/*	        Check for succesive keyword arguments
*               -------------------------------------
*/
                char * s;
                char * p;
                char * q;
                
                p = HTStrip(argv[arg]);

                for (q=p; *q; q++){
                    if (WHITE(*q)){
                        *q = '+';
                    }
                }  
                if (first_keyword){
                    s=strchr(chosen_reference,'?'); /* Find old search string */
                    if (s) *s =0;                   /* Chop old search off */
                    strcat(chosen_reference,"?");   /* Start new search */
                    first_keyword = NO;
                } else {
                    strcat(chosen_reference,"+");
                }
                strcat(chosen_reference,p);

            } /* Keywords */
        } /* Not an option '-'*/
    } /* End of arguement loop */

    if (!arguement_found){
	char * my_home = getenv(LOGICAL_DEFAULT);
	char * ref = HTParse(my_home ? my_home : DEFAULT_ADDRESS,
		    default_default,
		    PARSE_ACCESS|PARSE_HOST|PARSE_PATH|PARSE_PUNCTUATION);
	strcpy(chosen_reference, ref);
	free(ref);
    }

    StrAllocCopy(current_address,"");

    {   int k;

        for (k=0; k<SCREEN_WIDTH; k++) 
            space_string[k] = ' '; 
            space_string[SCREEN_WIDTH] = '\0'; 
    }

    if (Open_File(chosen_reference)){ 
        Make_History_Links();
    }

    if (FUNCTION_CHECK) printf("Initial call to Format_Lines");

    Format_Lines();	/* Initial action */

    while (YES) {

        if (FUNCTION_CHECK) printf("<main HTOutputLines>");

	HTOutputLines(); 

        if (FUNCTION_CHECK) printf("<main SelectionPrompt>");

        Selection_Prompt();
    }
} /* main() */

/************************/
/* End of main program  */
/************************/

/* ###########################################################################*/
/* ___________________________________________________________________________*/
/*
** Displays one page of text on the screen.
**
**    On exit,
**        If not EOF, produced a page of text on the screen.
*/

#ifdef __STDC__                             
void Format_Lines(void)
#else
void Format_Lines()
#endif   
                                                  
{
    if (FUNCTION_CHECK) printf("<Format_Lines>");

    for (line_count = 1; line_count<=(SCREEN_SIZE - 1);) { /* Incremented in
							      output_line */
	character = NEXT_CHAR;
	if (character != (char)EOF) {

	    Treat_One_Character();           

	} else {
 
	    end_of_file = YES;		     /* Flag it to change prompt */

            End_of_Unformatted_Text();

            break;

	}  /* if not EOF */
    }  /* for */

    return;
}
/* ___________________________________________________________________________*/
/* ___________________________________________________________________________*/
/*
**  Tries to open a file and if successful closes any remaining buffers 
**  containing unformatted text from previous files, and opens a new buffer
**  ready to read the file into so as to free the server as quickly as 
**  possible. Also initialises variables.
**
**    On Entry,
**        file_name  The address of the file to be accessed.
**
**    On Exit,
**
**        returns    1      Success in opening file
**                   0      Failure 
**
**        current_address   Contains the address of the present node being 
**                          viewed. 
**
**        Initialized :-    is_index,
**                          end_of_file,
**                          state, 
**                          current_style,
**                          change_indent(),
**                          blank_lines,
**                          href_count,
**                          name_count,
**                          position_in_line.
**
*/

#ifdef __STDC__
BOOL Open_File(const char * file_name)
#else
BOOL Open_File(file_name)
    char * file_name;
#endif

{
    char     * mycopy = 0;
    char     * stripped = 0;
    int	       new_file_number;
    WWW_Format format;


    StrAllocCopy(mycopy, file_name);

    stripped = HTStrip(mycopy);
 
    free(full_address);

    full_address = HTParse(stripped,
	           current_address,
		   PARSE_ACCESS|PARSE_HOST|PARSE_PATH|PARSE_PUNCTUATION);

    HTSimplify(full_address);

    new_file_number = HTOpen(full_address,&format);

    if (new_file_number<0) {		       /* Failure in accessing a file */

	printf("\nWWW: Can't access `%s'\n", full_address);
	if (!*current_address){
            exit(2);
        } else {
            return NO;
        }

    } else {				       /* Success in accessing a file */

        HTFormatBufferClose();
        HTBufferClose();                       /* Frees any remaining buffers */
        HTBufferOpen(new_file_number);
        first_page = YES;

	is_index = NO;			       /* Initialise as not an index */
	end_of_file = NO;		       /* Initialise as not E O F */

        if (TRACE) {
	    printf("WWW: Opened `%s' as fd %d\n",
	    full_address, new_file_number);
	}

        if (format==WWW_HTML) {
	    state = S_text;
            current_style = &normal_style;                 
        } else {    /* display as plain text */
	    state = S_plaintext;  /* Initialisation */  
            current_style = &mono_style;                 
	} /* plain text */

        end_of_file = NO;

	Change_Indent(1);                        
        blank_lines = 0;

        line_count = 0;  
        href_count = 0;
	name_count = 0;

	position_in_line = 0;		       /* Clear output line */

        title_found = NO;

	StrAllocCopy(current_address, full_address);
    
    }

    return YES;
}	
/* ___________________________________________________________________________*/
/* ___________________________________________________________________________*/
/*
**  Stores the address of the present node in a double linked circular
**  list so that the history of the nodes visited in the session can be 
**  recorded.
*/

#ifdef __STDC__
void Make_History_Links(void)
#else
void Make_History_Links()
#endif

{

    history *  present_node = (history *)malloc(sizeof(*present_node));

    present_node->title =0;         /* Non allocated as yet */

    if (home==0) {
	home = present_node;        /* Changes the home pointer from 0 aswell */
	home->next_node = home;
	home->prev_node = home;

    } else {
	present_node->next_node      = home;
       
	present_node->prev_node      = home->prev_node;

	home->prev_node->next_node   = present_node;

	home->prev_node              = present_node;           	    
  
    }
    
    present_node->address = (char * )malloc(strlen(current_address)+1);
    strcpy(present_node->address, current_address);

    node_count++;
  
}
/* ___________________________________________________________________________*/
/* ___________________________________________________________________________*/
/*
**  Moves a pointer to a document address in the history 
**  list, when jumping back to view a previously visited node.
**
**  On Entry,
**       ptr   Is set to point to the home address (first node visited).
**
**  On Exit,
**      ptr    Points to the address of the required node.
**
**      Required node is opened and the links in the history list made.
*/

#ifdef __STDC__
void Move_Pointer_Open_File_Link(int recall_node_number)
#else
void Move_Pointer_Open_File_Link(recall_node_number)
    int recall_node_number;
#endif

{
    history * ptr;
    int k;

    ptr=home;

    for (k=1; k<recall_node_number; k++){

        ptr = ptr->next_node;
    }

    Open_File(ptr->address);
    Make_History_Links();

    if (FUNCTION_CHECK) printf("Format_Lines called by MPOFL");

    Format_Lines();

}

/* ___________________________________________________________________________*/
/* ___________________________________________________________________________*/
/*
**  Moves the pointer back to the previous document address and opens the
**  file. 
*/

#ifdef __STDC__
void Backup_Pointer_Open_File(int recall_node_number)
#else
void Backup_Pointer_Open_File(recall_node_number)
#endif

{
    history * ptr;
    int k;

    ptr=home;

    for (k=1; k<recall_node_number; k++){

        ptr = ptr->next_node;
    }

    Open_File(ptr->address);
}

/* ___________________________________________________________________________*/
/* ___________________________________________________________________________*/
/*
**  Prints out a history list of all the nodes visited during the session. 
*/

#ifdef __STDC__
void History_List(void)
#else
void History_List()
#endif    

{
    int  history_number = 1; 


    printf("\n\n            HISTORY OF PREVIOUS NODES :- \n\n\n");

    {
        history * ptr;

       	ptr=home;

        do {

	    if (!ptr->title){
		printf("     %2d)       %s\n",history_number,ptr->address);
	    } else {
		printf("     %2d)       %s\n",history_number,ptr->title);
	    }

	    history_number++;
	    ptr = ptr->next_node;

	} while (ptr!=home);

        printf("\n\n\n");
                  
    }

    Selection_Prompt();

}      

/* ___________________________________________________________________________*/
/* ___________________________________________________________________________*/
/*
**  Backs up the history list to the previous node visited. This node is not 
**  relinked into the list and the address of the present node is unlinked. 
*/

#ifdef __STDC__
void History_Back(void)
#else
void History_Back()
#endif

{
    history * temp;

    temp=home->prev_node;

    temp->prev_node->next_node = home;  

    home->prev_node = home->prev_node->prev_node; 

    free(temp);
    free(temp->address);

    node_count--;

    Backup_Pointer_Open_File(node_count);

    if (FUNCTION_CHECK) printf("Format_Lines called by History_Back");

    Format_Lines();
}
/* ___________________________________________________________________________*/
/* ___________________________________________________________________________*/
/*
**  Deals with one character at a time, formatting according to the SGML tags.
**  Calls the function Character_Build(char *character), where each character 
**  is stored in a line to be output onto the screen.
*/

/* Displays the text on the screen looking for tags */

#ifdef __STDC__
void Treat_One_Character(void)
#else
void Treat_One_Character()
#endif

{
    PRIVATE  char  heading_level;    /* Stores the present level of the
                                        heading */
    PRIVATE  char  attribute[ADDRESS_LENGTH];  /* Temporary storage for
                                                  collecting a hypertext
                                                  reference */
    PRIVATE  int   attribute_count;  /* Counts the number of characters read
                                        when collecting a hypertext reference */
    PRIVATE  BOOL  href_found = NO;  /* Flags to indicate that an anchor has..*/

    PRIVATE  BOOL  name_found = NO;  /* been found and text is sensititive */

    PRIVATE  char  title_buffer[TITLE_LENGTH];  /* Temporary storage for the 
                                                   title of a node */ 
    PRIVATE  int   char_count;       /* Counts the number of characters in a 
                                        title */
    PRIVATE  BOOL  glossary = NO;    /* Flag to show that a glossary 
                                         is expected to follow */
    PRIVATE  char  test_tag_str[6];  /* Used for testing tags within xmp text */

/*
**    Parser 
**    ------
*/
    switch(state) {                          /* Switch statement on state */


    case S_text:                             /* Normal outputing of the text */

    	if (character=='<') {
            state=S_tag_start;
	} else {
	    Character_Build(character);
 	} 
	break;


    case S_tag_start:                        /* Examines the first character  
				                after the left angled bracket */
	switch(character) {

	case 't':                            /* Is tag <TITLE> ?*/
	case 'T':                            
	    state = Check("TITLE>") ? S_title : S_junk_tag;
	    if (state==S_title){
	        End_Style();
		Start_Style(&heading_style[0]);
	    }
	    break;

        case 'h':                        
       	case 'H': 
            state = S_tag_h;
            break;

	case 'i':
	case 'I':                           /* Is tag <ISINDEX> ?*/
	    if (Check("ISINDEX>")) {        /* Accepts keywords for searching */
		is_index = YES;
		state = S_text;
	    } else {
                state = S_junk_tag;
            }
	    break;

	case 'p':
       	case 'P':
            state = S_tag_p;
            break;

	case 'a':                           
	case 'A':
	    state = S_tag_a;
	    break;
   
	case 'o':                           /* Is tag <OL> (ordered list) ? */
	case 'O':                           
	case 'u':                           /* Is tag <UL> (unordered list) ? */
	case 'U':
	    if (toupper(character)=='O') {
                state=Check("OL>") ? S_text : S_junk_tag ;
	    }
	    if (toupper(character)=='U') {
	        state=Check("UL>") ? S_text : S_junk_tag ;
            }
            if (state == S_text){
                End_Style();
		Start_Style(&list_style);
            }                                      
	    break;

	case 'l':                   
	case 'L':                                           
            state = S_tag_l;
            break;

        case 'd':                      
        case 'D':
            state = S_tag_d ;
            break;

	case 'x':                        /* Is tag <XMP> (mono_spaced text) ? */
	case 'X':                        
	    state=Check("XMP>") ? S_xmp : S_junk_tag;
	    if (state == S_xmp){
                End_Style();
                Start_Style(&mono_style);
            } 
	    break;

	case '/':                         /* Back-slash at start of bracket.. */
	    state = S_tag_end;            /* indicates the end of a condition */
            break;

	default:
	    state=S_junk_tag;
	    break;

	}   /* switch(character) */
        break;   /* case S_tag_start */


    case S_title:
    	if (character=='<') {
            state=S_tag_start;
	} else {
	    Character_Build(character);
           
            if ((!title_found)&&(char_count<TITLE_LENGTH)){
		    title_buffer[char_count++] = character;
 	    }
 	} 
	break;


    case S_tag_h:                          

        switch(character){      

	case 'p':
	case 'P':
	    state=S_high_phrase;
	    break;

	case '0':                         
	case '1':	                                
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
            state = S_tag_h_num;
            heading_level = character;
            break;	    

	default:          
	    state=S_junk_tag;
	    break;

	}   /* switch (character) */                   
        break; /* case S_tag_h */


    case S_tag_h_p:

        if (character == '>'){            /* Is tag a highlighted phrase ? */
            state = S_high_phrase;
        } else {
            state = S_junk_tag;
        }
        break;    


    case S_tag_h_num:                     /* Is tag a heading ? */

        if (character == '>'){
	    Start_Style(&heading_style[heading_level - '0']);
	    state = S_text;
        } else {
            state = S_junk_tag;
        }
	break;	    

  
    case S_tag_p:

        switch(character) {

	case '>':                        /* Tag indicates a new paragraph */
	    Paragraph_End();
	    New_Line();
	    state = S_text;
	    break;

	 case 'l':
	 case 'L':
	     if (Check("LAINTEXT>")) {      /* Is tag <PLAINTEXT> ? */
	         End_Style();               /* Text displayed as it is read ..*/
 		 Start_Style(&mono_style);  /* straight from the file, not ..*/
		 state = S_plaintext;       /* in markup language (HTML/SGML) */
	     } else {
                 state = S_junk_tag;
             }
	     break;

	 default:
             state = S_junk_tag;
             break;

	 }  /* switch(character) */
	 break;  /* case S_tag_p */


    case S_tag_a:
        
        switch(character) {

        case 'd':
        case 'D':                                      /* Is tag <ADDRESS> ? */
            state = Check("DDRESS>") ? S_text : S_junk_tag;  
            if (state == S_text){                      /* Text right adjusted */
                End_Style();
		Start_Style(&address_style);
		state = S_text;
            } 
            break;

        case ' ':
            state = S_anchor;                          /* Tag is an ANCHOR */
            break;
  
        default:
            state = S_junk_tag;
            break;

        } /* switch(character) */     
        break; /* case S_tag_a */


    case S_tag_l:

        switch(character){
      
        case 'i':
        case 'I':
            state = S_tag_li;
            break;

        default:
            state = S_junk_tag;
            break;

        } /* switch(character) */
        break; /* case S_tag_l */

    
    case S_tag_li:
        
        switch(character){
  
        case '>':
            state = S_text;                /* Tag indicates a new list item */
	    End_Style();
	    Start_Style(&list_style);
            break;

        case 's':
        case 'S':                                       /* Is tag <LISTING> ? */
            state = Check("STING>") ? S_xmp : S_junk_tag; 
            if (state == S_xmp){                  
                End_Style();
                Start_Style(&mono_style);  /* Text in mono_spaced font */
            } 
            break;

        }  /* switch(character) */
        break;  /* case S_tag_li */


    case S_tag_d: 

	switch(character){
	    
	case 'l':                      
	case 'L':                              
	    state=Check("L>") ? S_text : S_junk_tag;     /* Is tag <DL> ? */
            if (state == S_text) {                       /* Start of glossary */
	        glossary = YES;                 
                End_Style();
	        Start_Style(&glossary_style);
            }
	    break;              

	case 't':                
        case 'T':                                        /* Is tag <DT> ? */
            state = Check("T>") ? S_text : S_junk_tag;   /* Start of a new ..*/
	    if (glossary == NO) state=S_junk_tag;        /* glossary term */
	    if (state == S_text) {
               Start_Style(&glossary_style);
	    }
	    break;

	case 'd':                
	case 'D':                                      /* Is tag <DD> ? */ 
	    state=Check("D>") ? S_text : S_junk_tag;   /* Start of a ..*/
	    if (glossary == NO) state=S_junk_tag;      /* glossary definition */
	    if (state == S_text){                
	        both_indent = YES;
		Change_Indent(2);
	    }
	    break;

	default :
	    state=S_junk_tag;
	    break;

	}  /* switch(character) */ 
	break;  /* case S_tag_d */   


    case S_tag_end:                 /* Checks tags starting with '/' to find
                                       the end of an existing condition */ 
        switch(character){ 
  
	case 't':                                         /* Is tag </TITLE> */
	case 'T':
            {
		history * present_node;

		state = Check("TITLE>") ? S_text : S_junk_tag;

		if (state == S_text){

		    title_buffer[char_count] = 0;
		    End_Style();

		    present_node = home->prev_node;

		    present_node->title = (char *)malloc(char_count+1);
		 
		    strcpy(present_node->title, title_buffer);

		    title_found = YES;
		    char_count = 0;
		}
            }
            break;

	case 'h':                           
	case 'H':
	    state = S_tag_end_h;
	    break;

	case 'u':                                 /* Is tag </UL> or </OL> ? */
	case 'U':                                 /* End of a list ? */
	case 'o':
	case 'O':
	    if (toupper(character)=='O') {                    
                state = Check("OL>") ? S_text : S_junk_tag;
                if (state == S_text){
                    End_Style();
                }
	    }
	    if (toupper(character)=='U') {
                state = Check("UL>") ? S_text : S_junk_tag;
                if (state == S_text){
                    End_Style();
                }
            }
	    break;

	case 'a':                        
	case 'A':
	    state = S_tag_end_a;
	    break;

	case 'd':                                      /* Is tag </DL> ? */
	case 'D':                                      /* End of a glossary ? */
	    state=Check("DL>") ?
		  S_text : S_junk_tag;   
	    if (state==S_text){
                End_Style();
                glossary = NO;
	    }
	    break;
 
	default:
	    state=S_junk_tag;
	    break;

	} /* switch(character) */
	break; /* case S_tag_end */
 

    case S_tag_end_h:              

   	switch(character){

	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	    state = S_tag_end_h_num;
	    break;

	default:
	    state = S_junk_tag;
	    break;

	}  /* switch(character) */
	break; /* case S_tag_end_h */


    case S_tag_end_h_num:                  /* Is tag the end of a heading ? */
	state = (character=='>') ? S_text : S_junk_tag;
        if (state == S_text) End_Style();
	break;

    case S_tag_end_a:               
	
        switch(character){
 
        case '>':                                  /* Tag is </A> */
	    {                                      /* End of anchor */ 
		char buff_str[50], *p;                       

		if (character=='>') {
		    if (href_found) {
			href_count++;
			if (display_anchors){
				  /* Inserts a number into text corresponding
				     to a hypertext reference */
			    sprintf(buff_str,"[%d]",href_count);
			} 
			for (p=buff_str; *p; p++)
			    Character_Build(*p);
			href_found = NO;    
			} 
	 	    if (name_found) {
	                name_found = NO;
                    }
	            state=S_text;
		}                        
	    }
	break;

        case 'd':
        case 'D':                                  /* Is tag </ADDRESS> ? */
            state = Check("DDRESS>") ? S_text : S_junk_tag;
            if (state == S_text){
          	End_Style();
		state = S_text;
            }        
            break;

        default:
            state = S_junk_tag;
            break;

        }  /* switch(character) */
        break;  /* case S_tag_end_a */



    case S_high_phrase:                     

	switch(character){

	case '1': 
            state = S_hp1;            
            break;

	case '2':
            state = S_hp2; 
            break;

	default:
	    state=S_junk_tag;
	    break; 
   
	}  /* switch(character) */
	break; /* case S_high_phrase */


    case S_hp1:
                                       /* Is tag <HP1> (highlighted phrase) ? */
	state=(Check(">") ? S_high_phrase1 : S_junk_tag );
	break;

 
    case S_hp2:
                                       /* Is tag <HP2> (highlighted phrase) ? */
	state=(Check(">") ? S_high_phrase2 : S_junk_tag );
	break;


    case S_high_phrase1:    
    case S_high_phrase2:
                                        /* Capitalizes text to be highlighted */
	if (character=='<'){
	    state=S_tag_start;
	} else {
	    Character_Build(toupper(character));
	}
	break;


    case S_anchor: 

	switch(character) {
            
 	case '\n':                                  /* Ignore white space */
        case '\r':
	case '\t':
	case ' ' :
	    break;	

	case 'h':                                   /* Is anchor an HREF ? */
	case 'H':                                    
	    attribute_count = 0;
	    state = (Check("HREF =") ? S_href : S_junk_tag);
	    break;

	case 'N':                                     /* Is anchor a NAME ? */
	case 'n':
	    attribute_count = 0;
	    state = (Check("NAME =") ? S_aname : S_junk_tag);
	    break;

	default:
	    state = S_junk_tag;
	    break;

	}  /* switch (character) */
	break;  /* case S_anchor */


   case S_href:                      /* Stores the reference if it is an HREF */

	if ((character!='>') && (!(WHITE_SPACE(character)))){
	    if (attribute_count< ADDRESS_LENGTH)
		attribute[attribute_count++] = character;
	    break;
	} 

	attribute[attribute_count] = '\0';
	href[href_count].address = (char *)malloc(attribute_count+1);  
	strcpy(href[href_count].address,HTStrip(attribute));

	href_found = YES;	     /* Flag that following text is sensitive */

	state = (character=='>') ? S_text : S_anchor;
	break;    


   case S_aname:                     /* Stores the reference if it is a NAME */

	if ((character!='>') && (!(WHITE_SPACE(character)))){
	    if (attribute_count< ADDRESS_LENGTH)
		attribute[attribute_count++] = character;
	    break;
	} 

	attribute[attribute_count] = '\0';
	name[name_count].address = (char *)malloc(attribute_count+1);  
	strcpy(name[name_count].address,HTStrip(attribute));

	name_found = YES;            /* Flag that following text is sensitive */

	state = (character=='>') ? S_text : S_anchor;

	break;    


    
    case S_plaintext :               /* Prints out text as it is read in the 
                                        read in the file (mono-spaced) */
	if ((character == '\n')||(character == '\r')||(character == '\t')) {
	    New_Line();
	} else {
	    if (character==' ') {
		line[position_in_line]=' ';
	        position_in_line++;        
	    } else {
		Character_Build(character);
	    }               
	}
	break;


    case S_xmp :                    /* Is tag <XMP> (monospaced text) ? */
 
        if (character == '<') {
            state = S_tag_in_xmp;
            test_tag_str[0] = character;
        } else {
	    if ((character=='\n')||(character=='\r')||(character=='\t')) {
                New_Line();
	    } else {
		if (character==' ') {
		    line[position_in_line]=' ';
		    position_in_line++;        
		} else {
		    Character_Build(character);
		}               
	    }
	}
	break;
         
    
    case S_tag_in_xmp:

        if (character == '/') {
            state = S_check_tag_in_xmp;
            test_tag_str[1] = character;
        } else {
            test_tag_str[1] = character;
            test_tag_str[2] = '\0';
            state = S_false_xmp_tag;            
        } 
        break;


    case S_check_tag_in_xmp:

        if ((character == 'x')||(character == toupper('x'))){
            state = S_check_tag_in_xmp_x;
            test_tag_str[2] = character;
        } else {
            test_tag_str[2] = character;
            test_tag_str[3] = '\0';
            state = S_false_xmp_tag;            
        }
        break;


    case S_check_tag_in_xmp_x:
        if ((character == 'm')||(character == toupper('m'))){
            state = S_check_tag_in_xmp_xm;
            test_tag_str[3] = character;
        } else {
            test_tag_str[3] = character;
            test_tag_str[4] = '\0';
            state = S_false_xmp_tag;            
        }
        break;


    case S_check_tag_in_xmp_xm:
        if ((character == 'p')||(character == toupper('p'))){
            state = S_check_tag_in_xmp_xmp;
            test_tag_str[4] = character;
        } else {
            test_tag_str[4] = character;
            test_tag_str[5] = '\0';
            state = S_false_xmp_tag;            
        }
        break;


    case S_check_tag_in_xmp_xmp:
        if (character == '>'){
            End_Style();
            state = S_text;
        } else {
            test_tag_str[5] = character;
            test_tag_str[6] = '\0';
            state = S_false_xmp_tag;            
        }
        break;


    case S_false_xmp_tag:
        {
           int k;

           for (k=0; test_tag_str[k] !='\0'; k++) 
               Character_Build(test_tag_str[k]);
           Character_Build(character);
           state = S_xmp;
        }
        break;


    case S_junk_tag:                /* Moves to the end of the angular bracket
                                       when an unknown tag is found */
	if (character =='>') state = S_text;
	break;

    }  /* switch(state) */

} /* end of function Treat_One_Character(character) */
/* ___________________________________________________________________________*/
/* ___________________________________________________________________________*/
/*
**  Checks that the charcters in the angled brackets are of correct form.
**
**  On Entry,
**       *s   Is the correct version of the tag, with which the actual tag is 
**            compared.
**
**  On Exit,
**       returns,   1  Tag is correct
**                  0  Tag is false
*/

#ifdef __STDC__
BOOL Check(char *s)
#else
BOOL Check(s)
    char *s;
#endif
   
{                                                     
    char *p = s+1;

    for (; *p; p++ ){
        if (*p == ' '){                            /* Check for excess spaces */
            while((character=NEXT_CHAR) ==' ');    /* Null */
            BACK_UP;                        /* Put non-blank back into stream */
        } else {
            character=NEXT_CHAR;

            if (toupper(character) != *p) {
                while (character!= '>') character=NEXT_CHAR; 
                return NO;
            }   /* if mismatch */
        }   /* if not space */
    }  /* for */
    return YES;
} 
/* ___________________________________________________________________________*/
/* ___________________________________________________________________________*/
/*
**  Accumulates characters and stores in a buffer untill the number accumulated
**  fills one line. These are then output on the screen and the buffer 
**  cleared.       
**
**  On Entry,
**      char character  The current character that is being parsed.
**
*/

#ifdef __STDC__
void Character_Build(char character) 
#else
void Character_Build(character)
    char character;
#endif
              
{
    char  CHARACTER;
 
    if ((both_indent)&&(begin_indent_change)){
                            /* Padding out of line with blank spaces in the
                               case where two left indents are used within the
                               same line e.g. glossary */
	if (((current_style->left_indent2)-(current_style->left_indent1))
				       -(position_in_line) > 0) {
	    while (position_in_line<((current_style->left_indent2) - 
                  (current_style->left_indent1))){
		line[position_in_line++]=' ';
	    }
	} else {  
	    New_Line();
	}
    }
    begin_indent_change = NO;
    
    if (!((both_indent)&&(position_in_line==(current_style->left_indent2)-
          (current_style->left_indent1)) && WHITE_SPACE(character))) {

	if (!((position_in_line==0)&& (WHITE_SPACE(character)))){
                                                 /* Prevents blank spaces  
                                                    at the beginning of lines */
	    if (character=='\n') character=' ';  /* Treats a new line as a 
				    	            blank space */

	    if (capitalize){
		CHARACTER=toupper(character);
		line[position_in_line++]=CHARACTER;
	    } else {
		line[position_in_line++]=character;
	    }
 
	    if (both_indent) {
                line_length = ((SCREEN_WIDTH) -
                               (current_style->right_margin_size) -
                               (current_style->left_indent1)); 
	    } else {
	        line_length = ((SCREEN_WIDTH) - 
                               (current_style->right_margin_size) -
                               (left_indent));
            }
            if ((current_style->double_spacing)&&(position_in_line<line_length))
      	    	line[position_in_line++]=' ';

	    if (position_in_line==line_length){

		Line_Wrap_and_Output();
	    }

	} 
    }
    return;
}
/* ___________________________________________________________________________*/
/* ___________________________________________________________________________*/
/*
**  Called when the number of characters in the line buffer is equal to the
**  allowed line length. Moves back through the line buffer untill it finds 
**  the first blank space between words. Function then calls Line_to_Buffer(),
**  and Excess_Line_Characters(), in which the line (up to the blank space) is
**  output and excess characters in the buffer (characters after the last
**  blank space) are copied to the start of the buffer and the rest of the 
**  buffer initialized, so they can be output in the next line on the screen,
**  when the buffer is full once again.
*/

#ifdef __STDC__
void Line_Wrap_and_Output(void)
#else
void Line_Wrap_and_Output()
#endif

{
    int  back_char = 0;


    if (state!=S_xmp){
 
        position_in_line = position_in_line-1;  

        while (line[position_in_line] ==' '){
            position_in_line = position_in_line-1; 
            back_char++;
        }
 
        if (current_style->double_spacing) {
            if (back_char<=1) {
                while (line[position_in_line]!=' '){
                    position_in_line=position_in_line-2;
                    if (position_in_line == 0){ 
                        position_in_line=line_length;            
                        break;
                    }
                }
            } else {
                position_in_line++;
            }    
        } else {
            if (back_char==0) {
                while (line[position_in_line]!=' '){
                    position_in_line=position_in_line-1;
                    if (position_in_line == 0){
                        position_in_line=line_length;   
                        break;
                    }
                }
            } else {
                position_in_line++;
            }
        }
    }

    line[position_in_line]='\0';  

    Line_to_Buffer();
    Excess_Line_Characters();

    return;
}
/* ___________________________________________________________________________*/
/* ___________________________________________________________________________*/
/*
**  Outputs the line contained within the buffer (up to the last blank space),
**  after first calculating the margin, which depends on the current_style and
**  the alignment.
*/ 

#ifdef __STDC__
void Line_to_Buffer(void)
#else
void Line_to_Buffer()
#endif

{
    PRIVATE  int   line_start, k;
             char  complete_line[SCREEN_WIDTH+1];

    if (current_style->alignment==0) {    /* Calculates where the line starts */
        if (both_indent) {
            line_start = current_style->left_indent1;
        } else {
            line_start = left_indent;
        }
    }
    if (current_style->alignment==1) {
        line_start = ((SCREEN_WIDTH) - (current_style->right_margin_size) - 
                      (position_in_line)); 
    }
    if (current_style->alignment==2) { 
        line_start =  ((line_length - (position_in_line + 1))/2 +  
                       left_indent); 
    }

    if (blank_lines != 0){               /* Blank lines before line in buffer */

	for (k=0;(k<blank_lines); k++){
	    HTBufferFormattedLine("");
	    line_count++;
        }
        blank_lines = 0;
    }

    sprintf(complete_line,"%s%s",SPACES(line_start),line);

    HTBufferFormattedLine(complete_line);

    line_count++;			                            

    both_indent = NO;                

    return;
}
/* ___________________________________________________________________________*/
/* ___________________________________________________________________________*/

/*
**  At the end of the file, when no unformatted characters remain, flushes out
**  the line into the formatted buffer and if interactive sends an end tag to 
**  in the output for the user.
*/

#ifdef _STDC_
void End_of_Unformatted_Text(void)
#else
void End_of_Unformatted_Text()
#endif

{
    if (FUNCTION_CHECK) printf("<End_of_Unformatted_Text>");

    End_Style();               /* Flushes line to formatted buffer */

    if (interactive){
 
        HTBufferFormattedLine("");
        HTBufferFormattedLine("     [End] ");

    }
}
/* ___________________________________________________________________________*/
/* ___________________________________________________________________________*/
/*
**  Copies excess characters (lines after the last blank space in the line 
**  buffer) to the start of the buffer and moves position_in_line, so that 
**  the buffer is ready to be over written with a new line of text.
*/ 

#ifdef __STDC__
void Excess_Line_Characters(void)
#else
void Excess_Line_Characters()
#endif

{
    if (position_in_line==line_length){
        position_in_line=0;
    } else {
    
	line[line_length]='\0';          
	position_in_line++;         
	while (line[position_in_line]==' ')          /* Checks the new line ..*/
            position_in_line=position_in_line + 1;   /* does not begin with ..*/
                                                     /* a blank space */
	strcpy(line,&line[position_in_line]);                       
	position_in_line=strlen(line);

	if ((current_style->double_spacing)&&(position_in_line!=0)){
	    if (line[position_in_line-1]!=' ') line[position_in_line++]=' ';
	}
    }
    return;

}
/* ___________________________________________________________________________*/
/* ___________________________________________________________________________*/
/*
**  Changes the current style.
**  
**  On Entry,
**       para_style *new_style   The new setting of the current style
*/

#ifdef __STDC__
void Start_Style(para_style *new_style)
#else
void Start_Style(new_style)
    para_style *new_style;
#endif

{
    Paragraph_End();                            /* Clears out line buffer */
    current_style = new_style;
    Change_Indent(1);                           

    if (blank_lines < current_style->lines_before)            
	blank_lines = current_style->lines_before;        

    return;

}
/* ___________________________________________________________________________*/
/* ___________________________________________________________________________*/
/*
**  Ends a style and returns to normal_style.
*/

#ifdef __STDC__
void End_Style(void)
#else
void End_Style()
#endif

{
    Paragraph_End();                            /* Clears out line buffer */

    if (blank_lines < current_style->lines_after) 
        blank_lines = current_style->lines_after;

    current_style = &normal_style;
    Change_Indent(1);

    return;
}
/* ___________________________________________________________________________*/
/* ___________________________________________________________________________*/
/* 
**  Clears out the line buffer onto the screen, if it is not already empty
**  before the number of characters is equal to the allowed line length.
**
**  On Exit,
**	 The line buffer is empty.
*/

#ifdef __STDC__
void Paragraph_End(void) 
#else
void Paragraph_End()
#endif

{
    if (position_in_line!=0){
        New_Line();
    }
}
/* ___________________________________________________________________________*/
/* ___________________________________________________________________________*/
/*
**  Clears out the line buffer onto the screen, before the number of characters
**  is equal to the allowed line length.
*/ 

#ifdef __STDC__
void New_Line(void) 
#else
void New_Line()
#endif

{

    line[position_in_line]='\0';

    Line_to_Buffer();
    
    position_in_line=0;

    return;

}
/* ___________________________________________________________________________*/
/* ___________________________________________________________________________*/
/*
**  Sets the current left indent from two possible values for each style.
**
**  On Entry,
**       int indent   Number which relates to the left indent setting.
**                    '1' being the left most setting
**                    '2' being the second setting.
**                    In most cases, '1' and '2' are the same values, except 
**                    in glossary_style.
*/

#ifdef __STDC__
void Change_Indent(int indent)
#else
void Change_Indent(indent)
    int indent;
#endif

{
    if (indent == 2) {
        left_indent = current_style->left_indent2;
        capitalize  = current_style->capitalize2;
    } else {
        left_indent = current_style->left_indent1;
        capitalize  = current_style->capitalize1;
    }

    begin_indent_change = YES;      /* Flag showing that this is first
                                       character read after the indent change */
    return;
}
/* ___________________________________________________________________________*/
/* ___________________________________________________________________________*/
/*
**   Produces a prompt at the bottom of a page full of text. The prompt varies 
**   depending on the options avaliable.
*/

#ifdef __STDC__
void Selection_Prompt(void)
#else
void Selection_Prompt()
#endif

{ 
    int length_of_prompt = 0;

    if (FUNCTION_CHECK) printf("<Selection_Prompt>");

    if (end_of_file && (href_count==0) && !is_index 
          && (current_address == home->address)){
        printf("\n");      
	exit(0);		             /* Exit if no other options */
    }

    if (is_index){	
        printf("K <keywords>, ");
        length_of_prompt = length_of_prompt + 14;
    }
    if (href_count!=0){
	printf("<ref.number>, ");	
        length_of_prompt = length_of_prompt + 14;
    }
    if (node_count>1){
        printf("'Back', ");
        length_of_prompt = length_of_prompt + 8;
    }
    if (!end_of_file){
        printf("<RETURN> for more, ");
        length_of_prompt = length_of_prompt + 19;
    }
    if (length_of_prompt <= 47){
        printf("'Quit', ");
    }
    printf("or H for help: ");	

    if (first_page == YES) {
        HTBufferFile();
        first_page = NO;
    }

    User_Selection();

    return;
}
/* ___________________________________________________________________________*/
/* ___________________________________________________________________________*/
/*
**  Reads in the users input, and deals with as necessary.
*/

#ifdef __STDC__
void User_Selection(void)
#else
void User_Selection()
#endif

{   
    int  reference_num; 
    int  k = 0;

#ifdef NEWLINE_PROMPT
    printf("\n");               /* For use on VM to flush out the prompt ? */ 
#endif

    fgets(choice,RESPONSE_LENGTH,stdin);

    while (choice[k] == ' ') k++;


    switch(choice[k]) {

    case '0':                               /* Selection of a reference */
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        sscanf(choice,"%d",&reference_num);
        if ((reference_num<1)||(reference_num>href_count)){
            Error_Selection();
            Help_Menu();
        } else {
            Select_Reference(reference_num);
        }            
	break;


    case '\n':                              /* Continue with next page of   
                                               document */
        if (bottom_of_formatted_buffer){
	    Selection_Prompt();        
        }
	break;


    case 'h':                             
    case 'H':

        switch(choice[k+1]){
 
        case '\n':                          /* Assumes user requires the ..*/
            Help_Menu();                    /* help menu */
            break;

        case 'e':
        case 'E':
            if (Check_User_Input("HELP")){  /* Does user require help menu, ..*/
                Help_Menu();                /* or a keyword search ? */ 
            } else {
                if (is_index){
                    Search(&choice[k]);
                } else {  
                    Error_Selection();
                }
            }
            break;

        case 'o':
        case 'O':
            if (Check_User_Input("HOME")){ /* Does user require to return to
                                              home node, or a keyword search */
		if (node_count<2){ 
		    printf("\n\n   You are already in the HOME document");
		    printf(" which is :-\n\n");   
		    printf("   %s\n\n",home->address);
		    Selection_Prompt();
                } else {
		    Move_Pointer_Open_File_Link(1);
                }
            } else {
		if (is_index){
		    Search(&choice[k]);
		} else {
		    Error_Selection();
		}
	    }
            break;

 
        default:
            if (is_index){              /* Keyword search if avaliable, or ..*/ 
                Search(&choice[k]);     /* error message */
            } else {
                Error_Selection();
            }
            break;
   
        }    /* End of switch on choice[k+1] */
        break;
   
    case 't':
    case 'T':
        if (Check_User_Input("TOP")){
            HTTopofNode();
        } else {
	    if (is_index){
		Search(&choice[k]);
	    } else {
		Error_Selection();
	    }
	}
	break;


    case 'r':                            
    case 'R':
	{
	    int  recall_node_num;
	    int status = 0;
	    char * p;

            if (Check_User_Input("RECALL")){     /* Does user require RECALL */
		if (node_count<2){               /* No nodes to recall */
		    printf("\n\n   The RECALL command cannot be used");
		    printf(" as there are no previous documents\n");
		    printf("   The present document is :-\n");
		    printf("   %s\n\n",home->address);

		    Selection_Prompt();

   	        } else {                        

		    p = strchr(choice, ' ');
         	                      /* Is there a previous node number to 
	                                 recall, or does the user just require
                                         a list of nodes visited */
		    if (p) status = sscanf(p, "%d",&recall_node_num);

		    if (status == 1){

                        if(recall_node_num == node_count){
                            HTTopofNode();
                         } else {
                            if (recall_node_num == (node_count - 1)){
                                History_Back();
                            } else {
			        if ((recall_node_num>=1)&&
			           (recall_node_num<=node_count)){

                                       Move_Pointer_Open_File_Link
                                           (recall_node_num);

				} else { 
			            Error_Selection();
                                }
                            }   
			}
		    } else {
			if (status == 0){
			    History_List();
			} else {
			    Error_Selection();
			}
		    }
	        }  /* if node_count<2 */
	    } else {
	    	if (is_index){
		    Search(&choice[k]);
		} else {
		    Error_Selection();
		}
	    }  /* if CHECK("RECALL") */
	}	/* scope of status */
	break;



    case 'b':                    
    case 'B':                    
                                          /* Return to previous node ? */
        if (Check_User_Input("BACK")) {
	    if (node_count<2){            /* No nodes to jump back to */
		printf("\n\n   The BACK command cannot be used,");
		printf(" as there are no previous documents");
		printf("\n   The present document is :-\n");
		printf("   %s\n\n",home->address);
                Selection_Prompt(); 
	    } else {
		    History_Back();
            }  /* If (node_count<2) */
	} else {
	    if (is_index){
		Search(&choice[k]);
	    } else {
		Error_Selection();
	    }
	}
        break;


    case 'l':                               /* List of references ? */
    case 'L':
        if (Check_User_Input("LIST")){
            Reference_List();
        } else { 
	    if (is_index){
		Search(&choice[k]);
	    } else {
		Error_Selection();
	    }
        }
        break;



    case 'q':                                /* Quit program ? */
    case 'Q':
        if (Check_User_Input("QUIT")){
            exit(0);
        } else { 
	    if (is_index){
		Search(&choice[k]);
	    } else {
		Error_Selection();
	    }
        }
        break;



    case 'e':                                /* Quit program ? */
    case 'E':                                /* Alternative command */
        if (Check_User_Input("EXIT")){
            exit(0);
        } else { 
	    if (is_index){
		Search(&choice[k]);
	    } else {
		Error_Selection();
	    }
        }
        break;


    case 'k':                                /* Keyword search ? */
    case 'K':
        {  
	    if (Check_User_Input("KEYWORDS")){
		while (choice[k]!=' ') k++;
		while (choice[k]==' ') k++;
		Search(&choice[k]);
	    } else { 
		Error_Selection();
	    }
        }
        break;

      
    default :
        if (is_index){                       
            Search(&choice[k]);
        } else {             
            Error_Selection();
        }
        break;
    }
}
/* ___________________________________________________________________________*/
/* ___________________________________________________________________________*/
/*
**  Checks users input. Accepts shortened versions of commands.
**
**  On Entry,
**       char *s  Correct and full version of command, to which the users 
**                input is compared.
**
**  On Exit,
**         returns  YES  Users input corresponds to the command.
**                   NO  Not a know command.  
*/

#ifdef __STDC__
BOOL Check_User_Input(char *s)
#else
BOOL Check_User_Input(s)
    char *s;
#endif
   
{   int k=0;
    int match=0;

    while (choice[k] == ' ') k++;

    for (; *s; s++){

        if (*s == toupper(choice[k])){
            match++;
            k++;
        } else {
            if (((choice[k] == '\n')||(choice[k] == ' '))&&(match>0)){
                return YES;          
            } else {
                return NO;
            }
        }
    }
    return YES;
}     
/* ___________________________________________________________________________*/
/* ___________________________________________________________________________*/
/*
** Produces an error message if user input is not recognised
*/                       

#ifdef __STDC__
void Error_Selection(void)
#else
void Error_Selection()
#endif

{    
    printf("%s", "         *** INCORRECT SELECTION *** \n");
 
    return;
}
/* ___________________________________________________________________________*/
/* ___________________________________________________________________________*/
/*
** Produces a help menu of a list of available commands. 
*/

#ifdef __STDC__
void Help_Menu(void)
#else
void Help_Menu()
#endif

{
    printf("\n\n    <<< COMMANDS AVALIABLE ON THE WORLD WIDE WEB LINE BROWSER");
    printf(" Version 0.11 >>> \n\n");
    printf("    You are currently at address :\n    '%s' \n\n",full_address);
    printf("    <RETURN>     Produces the next page of the remaining\n");
    printf("                 text.\n");
    printf("    TOP          Returns to the first page of the present \n");
    printf("                 document.\n");
    if (href_count != 0) {
        printf("    LIST	 Produces a list of hypertext references\n");
        printf("                 which have been accumulated from the text\n");
        printf("                 already shown on the screen.\n");
        printf("    <number>     Select a referenced document by number\n");
	printf("                 (from 1 to %i)\n",
							    href_count);
    }
    if (is_index) {
	printf("    K <keywords> Search this index for given keywords\n"); 
	printf("                 Keywords are separated by spaces.\n");
    }
    if (node_count>1) {
        printf("    RECALL       Gives a list of the previous nodes\n");    
        printf("                 visited.\n");
        printf("    RECALL <number>\n");
        printf("                 Returns to a previously visited document\n");
        printf("                 numbered in the recall list.\n");
        printf("    HOME         Returns to the starting node\n");
        printf("    BACK         Moves back to the previous node\n");
    }
    printf    ("    Q            Quits the program.\n");
    printf("\n");

    Selection_Prompt();
    
    return;
}
/* ___________________________________________________________________________*/
/* ___________________________________________________________________________*/
/* 
**  Prints out a list of Hyper Text References accumulated within the text.
*/

#ifdef __STDC__
void Reference_List(void)
#else
void Reference_List()
#endif

{
    int  count;
    int  n;


    if (href_count == 0) {
        printf("\n\n     NO HYPERTEXT REFERENCES HAVE APPEARED");
        printf(" IN THE DOCUMENT YET\n\n\n");
    } else {

	printf("\n\n      HYPERTEXT REFERENCES := \n\n");
	
	count=1;

	for (n=0; n<href_count; n++) {

	    printf("     [%d]       %s\n\n", count,href[n].address);
	    count++;
	}
        printf("\n\n");

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
/* Check to see that the NAMES in the anchors have been also stored           */

	if (NAME_CHECK) {
	    
	    count=1;
	    for (n=0 ;n<name_count; n++) {
		printf("     [%d]       %s\n\n", count,name[n].address);
		count++;
	    }   
            printf("\n\n");
	}
/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/
    }

    Selection_Prompt();
}      
/* ___________________________________________________________________________*/
/* ___________________________________________________________________________*/
/*
**  Opens. links into the history list and displays a reference selected be
**  the user.
**
**  On Entry,
**       int  reference_num   Number corresponding to the hypertext reference
**                            given in the text.
*/

#ifdef __STDC__
void Select_Reference(int  reference_num)
#else
void Select_Reference(reference_num)
    int reference_num;
#endif

{  
    char * parsed_address;

    strcpy(chosen_reference, href[reference_num - 1].address);

    parsed_address = HTParse(chosen_reference,
		             current_address,
		             PARSE_ACCESS|
                             PARSE_HOST|
                             PARSE_PATH|
                             PARSE_PUNCTUATION);

    if (0==strcmp(parsed_address,current_address)){
        HTTopofNode();
    } else {
	printf("\n");

	if (Open_File(chosen_reference)){
	    Make_History_Links();
	} else {
	    printf("\n");
	    Selection_Prompt();
	}
    }
    free(parsed_address);

}
/* ___________________________________________________________________________*/
/* ___________________________________________________________________________*/
/*
**  Performs a keyword search on word given by the user. Adds the keyword to 
**  the end of the current address and attempts to open the new address.
**
**  On Entry,
**       char  *keywords  Word given by the user after the selection prompt
*/

#ifdef __STDC__
void Search(char * keywords)
#else
void Search(keywords)
    char * keywords;
#endif

{
    char * p;	    /* point to first non-blank */
    char * q, *s;

    p = HTStrip(keywords);
    for (q=p; *q; q++)
        if (WHITE(*q)) {
	    *q = '+';
	}
    strcpy(chosen_reference, current_address);

    s=strchr(chosen_reference, '?');		/* Find old search string */
    if (s) *s = 0;			/* Chop old search off */

    strcat(chosen_reference, "?");
    strcat(chosen_reference, p);

    if (Open_File(chosen_reference)){
        Make_History_Links();

        if (FUNCTION_CHECK) printf("Format_Lines called by Search");

        Format_Lines();
    } else {
        Error_Selection();
    }
}
/* ___________________________________________________________________________*/
