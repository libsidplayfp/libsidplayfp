//
// STILView - command line version
//

#include <fstream.h>
#include <iostream.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "stil.h"

STIL myStil;
char *hvscLoc = NULL;
char *entryStr = NULL;
int tuneNo = 0;
STIL::STILField field = STIL::all;
bool showBug = true;
bool showEntry = true;
bool showSection = true;
bool showVersion = false;
bool interactive = false;
bool demo = false;

char *STIL_DEMO_ENTRY="/Galway_Martin/Green_Beret.sid";

// This is used for testing setBaseDir() when switching between different
// HVSC base directories. Ideally, it should point to a valid HVSC dir.
char *OTHER_HVSC_BASE_DIR="E:\\MUSIC\\SID\\C64music\\";

#define STIL_MAX_PATH_SIZE 1024

void printUsageStr(void)
{
    cout << endl;
    cout << myStil.getVersion();
    cout << "USAGE: STILView [-e=<entry>] [-l=<HVSC loc>] [-t=<tuneNo>] [-f=<field>]" << endl;
    cout << "                [-d] [-i] [-s] [-b] [-o] [-v] [-h] [-m]" << endl;
}

void printUsage(void)
{
    printUsageStr();
    exit(1);
}

void printHelp(void)
{
    printUsageStr();
    cout << "Arguments can be specified in any order." << endl;
    cout << endl;
    cout << "-e=<entry>    - Specifies the desired STIL entry with an HVSC-relative path." << endl;
    cout << "-l=<HVSC loc> - Specifies the location of the HVSC base directory. If not" << endl;
    cout << "                specified, the value of the HVSC_BASE env. variable will be" << endl;
    cout << "                used. Specifying this option will override HVSC_BASE." << endl;
    cout << "-t=<tuneNo>   - If specified, only the STIL entry for the given tune number is" << endl;
    cout << "                printed. (Default: 0)" << endl;
    cout << "-f=<field>    - If specified, only the STIL entry for the given field is" << endl;
    cout << "                printed. (Default: all)" << endl;
    cout << "                Valid values for <field> are:" << endl;
    cout << "                all, name, author, title, artist, comment" << endl;
    cout << "-d            - Turns on debug mode for STILView." << endl;
    cout << "-i            - Enter interactive mode." << endl;
    cout << "-m            - Demo mode (tests STILView and shows its capabilities)." << endl;
    cout << "-s            - If specified, section-global (per dir/per composer) comments" << endl;
    cout << "                will NOT be printed." << endl;
    cout << "-b            - If specified, BUG entries will NOT be printed." << endl;
    cout << "-o            - If specified, STIL entries will NOT be printed." << endl;
    cout << "-v            - Print STILView's and STIL's version number." << endl;
    cout << "-h            - Print this help screen (all other options are ignored)." << endl;
    cout << endl;
    cout << "See USAGE.txt for further details and for examples." << endl;
    cout << endl;
    exit(0);
}

char *getArgValue(char *argStr)
{
    char *temp;

    temp = (char *)strchr(argStr, '=');

    if (temp == NULL) {
        return NULL;
    }

    if (*(temp+1) == '\0') {
        return NULL;
    }

    return (temp+1);
}

void processArguments(int argc, char **argv)
{
    int i;
    char *fieldStr;
    char *tuneStr;
    char *tempLoc;

    for (i=1; i<argc; i++) {
        if (argv[i][0] == '-') {
            switch (argv[i][1]) {
                case 'd':
                case 'D':
                    myStil.STIL_DEBUG = true;
                    break;
                case 'e':
                case 'E':
                    entryStr = getArgValue(argv[i]);
                    if (entryStr == NULL) {
                        cerr << "ERROR: STIL entry was not specified correctly!" << endl;
                        printUsage();
                    }
                    break;
                case 'i':
                case 'I':
                    interactive = true;
                    break;
                case 'm':
                case 'M':
                    demo = true;
                    break;
                case 'l':
                case 'L':
                    tempLoc = getArgValue(argv[i]);
                    if (tempLoc != NULL) {
                        hvscLoc = tempLoc;
                    }
                    break;
                case 't':
                case 'T':
                    tuneStr = getArgValue(argv[i]);
                    if (tuneStr == NULL) {
                        cerr << "ERROR: tune number was not specified correctly!" << endl;
                        printUsage();
                    }
                    sscanf(tuneStr, "%d", &tuneNo);
                    break;
                case 's':
                case 'S':
                    showSection = false;
                    break;
                case 'b':
                case 'B':
                    showBug = false;
                    break;
                case 'o':
                case 'O':
                    showEntry = false;
                    break;
                case 'v':
                case 'V':
                    showVersion = true;
                    break;
                case 'f':
                case 'F':
                    fieldStr = getArgValue(argv[i]);
                    if (fieldStr == NULL) {
                        cerr << "ERROR: field was not specified correctly!" << endl;
                        printUsage();
                    }
                    if (MYSTRICMP(fieldStr, "all") == 0) {
                        field = STIL::all;
                    }
                    else if (MYSTRICMP(fieldStr, "name") == 0) {
                        field = STIL::name;
                    }
                    else if (MYSTRICMP(fieldStr, "author") == 0) {
                        field = STIL::author;
                    }
                    else if (MYSTRICMP(fieldStr, "title") == 0) {
                        field = STIL::title;
                    }
                    else if (MYSTRICMP(fieldStr, "artist") == 0) {
                        field = STIL::artist;
                    }
                    else if (MYSTRICMP(fieldStr, "comment") == 0) {
                        field = STIL::comment;
                    }
                    else {
                        cerr << "ERROR: Unknown STIL field specified: '" << fieldStr << "' !" << endl;
                        cerr << "Valid values for <field> are:" << endl;
                        cerr << "all, name, author, title, artist, comment." << endl;
                        printUsage();
                    }
                    break;
                case 'h':
                case 'H':
                    printHelp();
                    break;
                default:
                    cerr << "ERROR: Unknown argument: '" << argv[i] << "' !" << endl;
                    printUsage();
                    break;
            }
        }
        else {
            cerr << "ERROR: Unknown argument: '" << argv[i] << "' !" << endl;
            printUsage();
        }
    }
}

void checkArguments(void)
{
    if (hvscLoc == NULL) {
        if (interactive || demo) {
            hvscLoc = new char[STIL_MAX_PATH_SIZE];
            cout << "Enter HVSC base directory: ";
            cin >> hvscLoc;
        }
        else {
            if (showVersion) {
                showBug = false;
                showEntry = false;
                showSection = false;
            }
            else {
                cerr << "ERROR: HVSC base dir was not specified and HVSC_BASE is not set, either!" << endl;
                printUsage();
            }
        }
    }

    if (entryStr == NULL) {
        if ((!interactive) && (!demo)) {
            if (showVersion) {
                showBug = false;
                showEntry = false;
                showSection = false;
            }
            else {
                cerr << "ERROR: STIL entry was not specified!" << endl;
                printUsage();
            }
        }
        else {
            entryStr = STIL_DEMO_ENTRY;
        }
    }
}

int main(int argc, char **argv)
{
    char temp[STIL_MAX_PATH_SIZE];
    char *tmpptr, *versionPtr, *sectionPtr, *entryPtr, *bugPtr;
    char fieldchar;
    float tempval;

    if (argc < 2) {
        printHelp();
    }

    hvscLoc = getenv("HVSC_BASE");

    processArguments(argc, argv);

    checkArguments();

    if (interactive || demo) {
        cout << "Reading STIL..." << endl;
    }
    else {
        if (showVersion && (hvscLoc == NULL)) {
            tmpptr = myStil.getVersion();
            if (tmpptr == NULL) {
                cerr << "ERROR: No STIL version string was found!" << endl;
            }
            else {
                cout << tmpptr;
            }

            exit(0);
        }
    }

    if (myStil.setBaseDir(hvscLoc) != true) {
        cerr << "STIL error #" << myStil.getError() << ": " << myStil.getErrorStr() << endl;
        exit(1);
    }

    if ((!interactive) && (!demo)) {

        // Pure command-line version.

        if (showVersion) {
            versionPtr = myStil.getVersion();
        }
        else {
            versionPtr = NULL;
        }

        if (showSection) {
            sectionPtr = myStil.getGlobalComment(entryStr);
        }
        else {
            sectionPtr = NULL;
        }

        if (showEntry) {
            entryPtr = myStil.getEntry(entryStr, tuneNo, field);
        }
        else {
            entryPtr = NULL;
        }

        if (showBug) {
            bugPtr = myStil.getBug(entryStr, tuneNo);
        }
        else {
            bugPtr = NULL;
        }

        if (versionPtr != NULL) {
            if ((sectionPtr != NULL) || (entryPtr != NULL) || (bugPtr != NULL)) {
                cout << "--- STILView  VERSION ---" << endl;
            }
            cout << versionPtr;
        }

        if (sectionPtr != NULL) {
            if ((versionPtr != NULL) || (entryPtr != NULL) || (bugPtr != NULL)) {
                cout << "---- GLOBAL  COMMENT ----" << endl;
            }
            cout << sectionPtr;
        }

        if (entryPtr != NULL) {
            if ((versionPtr != NULL) || (sectionPtr != NULL) || (bugPtr != NULL)) {
                cout << "------ STIL  ENTRY ------" << endl;
            }
            cout << entryPtr;
        }

        if (bugPtr != NULL) {
            if ((versionPtr != NULL) || (sectionPtr != NULL) || (entryPtr != NULL)) {
                cout << "---------- BUG ----------" << endl;
            }
            cout << bugPtr;
        }
    }
    else {

        // We are either in interactive or demo mode here.

        if (demo) {
            cout << "==== STILVIEW  DEMO MODE ====" << endl;
            cout << endl << "---- STIL VERSION ----" << endl;
            cout << "---- ONE STRING ----" << endl;
        }

        // This gets printed regardless.

        tmpptr = myStil.getVersion();
        if (tmpptr == NULL) {
            cerr << "ERROR: No STIL version string was found!" << endl;
        }
        else {
            cout << tmpptr;
        }

        if (demo) {

            // Demo mode.

            cout << "---- STIL CLASS VERSION # ----" << endl;
            tempval = myStil.getVersionNo();
            if (tempval == 0) {
                cerr << "ERROR: STILView version number was not found!" << endl;
            }
            else {
                cout << "STILView v" << tempval << endl;
            }

            cout << "---- STIL.txt VERSION # ----" << endl;
            tempval = myStil.getSTILVersionNo();
            if (tempval == 0) {
                cerr << "ERROR: STIL version number was not found!" << endl;
            }
            else {
                cout << "STIL v" << tempval << endl;
            }

            // For testing setBaseDir().

            if (myStil.STIL_DEBUG == true) {
                if (myStil.setBaseDir(OTHER_HVSC_BASE_DIR) != true) {
                    cerr << "STIL error #" << myStil.getError() << ": " << myStil.getErrorStr() << endl;
                    cerr << "Couldn't switch to new dir: '" << OTHER_HVSC_BASE_DIR << "'" << endl;
                    cerr << "Reverting back to '" << hvscLoc << "'" << endl;
                }
                else {
                    hvscLoc = OTHER_HVSC_BASE_DIR;

                    cout << "Switch to new dir '" << hvscLoc << "' was successful!" << endl;

                    cout << "---- ONE STRING ----" << endl;

                    tmpptr = myStil.getVersion();
                    if (tmpptr == NULL) {
                        cerr << "ERROR: No STIL version string was found!" << endl;
                    }
                    else {
                        cout << tmpptr;
                    }

                    cout << "---- STIL CLASS VERSION # ----" << endl;
                    tempval = myStil.getVersionNo();
                    if (tempval == 0) {
                        cerr << "ERROR: STILView version number was not found!" << endl;
                    }
                    else {
                        cout << "STILView v" << tempval << endl;
                    }

                    cout << "---- STIL.txt VERSION # ----" << endl;
                    tempval = myStil.getSTILVersionNo();
                    if (tempval == 0) {
                        cerr << "ERROR: STIL version number was not found!" << endl;
                    }
                    else {
                        cout << "STIL v" << tempval << endl;
                    }
                }
            }

            cout << endl << "==== STIL ABSOLUTE PATH TO " << entryStr << ", Tune #" << tuneNo << " ====" << endl << endl;

            strcpy(temp, hvscLoc);

            // Chop the trailing slash
            tmpptr = temp+strlen(temp)-1;
            if (*tmpptr == SLASH) {
                *tmpptr = '\0';
            }
            strcat(temp, entryStr);

            cout << "---- GLOBAL  COMMENT ----" << endl;

            tmpptr = myStil.getAbsGlobalComment(temp);

            if (tmpptr == NULL) {
                cerr << "STIL error #" << myStil.getError() << ": " << myStil.getErrorStr() << endl;
            }
            else {
                cout << tmpptr;
            }

            cout << "-- TUNE GLOBAL COMMENT --" << endl;

            tmpptr = myStil.getAbsEntry(temp, 0, STIL::comment);

            if (tmpptr == NULL) {
                cerr << "STIL error #" << myStil.getError() << ": " << myStil.getErrorStr() << endl;
            }
            else {
                cout << tmpptr;
            }

            cout << "------ STIL  ENTRY ------" << endl;
            cout << "(For tune #1)" << endl;

            tmpptr = myStil.getAbsEntry(temp, 1, STIL::all);

            if (tmpptr == NULL) {
                cerr << "STIL error #" << myStil.getError() << ": " << myStil.getErrorStr() << endl;
            }
            else {
                cout << tmpptr;
            }

            cout << "---------- BUG ----------" << endl;

            tmpptr = myStil.getAbsBug(temp, tuneNo);

            if (tmpptr == NULL) {
                cerr << "STIL error #" << myStil.getError() << ": " << myStil.getErrorStr() << endl;
            }
            else {
                cout << tmpptr;
            }

            cout << "==== END OF ENTRY ====" << endl;

            cout << endl << "Trying to do setBaseDir() to wrong location..." << endl;

            if (myStil.setBaseDir("This_should_not_work") != true) {
                cout << "setBaseDir() failed!" << endl;
                cout << "But it should't have an impact on private data!" << endl;
                cout << "You should see the same entry below:" << endl;
                cout << endl << "------ STIL  ENTRY ------" << endl;

                tmpptr = myStil.getAbsEntry(temp, tuneNo, STIL::all);

                if (tmpptr == NULL) {
                    cerr << "STIL error #" << myStil.getError() << ": " << myStil.getErrorStr() << endl;
                }
                else {
                    cout << tmpptr;
                }
            }
            else {
                cout << "Oops, it should've failed!" << endl;
            }
        }

        if (interactive) {

            // Interactive mode.

            cout << endl << "==== ENTERING INTERACTIVE MODE ====" << endl << endl;

            do {
                cout << "Enter desired entry (relative path) or 'q' to exit." << endl;
                cout << "Entry: ";
                cin >> temp;

                if (*temp == '/') {
                    cout << "Enter tune number (can enter 0, too): ";
                    cin >> tuneNo;

                    cout << "Field [(A)ll, (N)ame, A(U)thor (T)itle, A(R)tist,(C)omment]: ";
                    cin >> fieldchar;

                    switch (fieldchar) {
                        case 'a':
                        case 'A':
                            field = STIL::all;
                            break;
                        case 'n':
                        case 'N':
                            field = STIL::name;
                            break;
                        case 'u':
                        case 'U':
                            field = STIL::author;
                            break;
                        case 't':
                        case 'T':
                            field = STIL::title;
                            break;
                        case 'r':
                        case 'R':
                            field = STIL::artist;
                            break;
                        case 'c':
                        case 'C':
                            field = STIL::comment;
                            break;
                        default:
                            cout << "Wrong field. Assuming (A)ll." << endl;
                            field = STIL::all;
                            break;
                    }

                    cout << endl << "==== " << temp << ", Tune #" << tuneNo << " ====" << endl << endl;
                    cout << "---- GLOBAL  COMMENT ----" << endl;

                    tmpptr = myStil.getGlobalComment(temp);

                    if (tmpptr) {
                        cout << tmpptr;
                    }
                    else {
                        cout << "NONE!" << endl;
                    }

                    cout << "------ STIL  ENTRY ------" << endl;

                    tmpptr = myStil.getEntry(temp, tuneNo, field);

                    if (tmpptr) {
                        cout << tmpptr;
                    }
                    else {
                        cout << "NONE!" << endl;
                    }

                    cout << "---------- BUG ----------" << endl;

                    tmpptr = myStil.getBug(temp, tuneNo);

                    if (tmpptr) {
                        cout << tmpptr;
                    }
                    else {
                        cout << "NONE!" << endl;
                    }

                    cout << "==== END OF ENTRY ====" << endl << endl;
                }
            } while (*temp == '/');

            cout << "BYE!" << endl;
        }
    }

    return 0;
}
