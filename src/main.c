#define _XOPEN_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include "structs.h"

Schedule emptySchedule;
Event emptyEvent;
Class emptyClass;
const char *DayNames[] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

Schedule s;
int numEvents = 0;
int numClasses = 0;

/* ---------------------------------------------------------------------
   Utilities: minutes conversion, 12/24hr helpers, safe input helpers
   --------------------------------------------------------------------- */
int toMinutes(int h, int m) {
    return h * 60 + m;
}

void fromMinutes(int total, int *h, int *m) {
    if (total < 0) total = 0;
    if (total > 24*60 - 1) total = 24*60 - 1;
    *h = total / 60;
    *m = total % 60;
}

int to24Hour(int hour12, char ampm) {
    // hour12 in 1..12
    if ((ampm == 'A' || ampm == 'a')) {
        if (hour12 == 12) return 0;
        return hour12;
    } else {
        // PM
        if (hour12 == 12) return 12;
        return hour12 + 12;
    }
}

void print12Hour(int hour24, int minute) {
    int displayHour = hour24 % 12;
    if (displayHour == 0) displayHour = 12;
    printf("%2d:%02d %s", displayHour, minute, (hour24 < 12 ? "AM" : "PM"));
}

void readAMPMTime(const char *promptHour, int *outH24, int *outM) {
    int h12, m;
    char ampm;
    while (1) {
        printf("%s (hour 1-12): ", promptHour);
        if (scanf("%d", &h12) != 1) { scanf("%*s"); continue; }
        if (h12 < 1 || h12 > 12) { printf("Invalid hour; try again.\n"); continue; }
        break;
    }
    while (1) {
        printf("Minute (0-59): ");
        if (scanf("%d", &m) != 1) { scanf("%*s"); continue; }
        if (m < 0 || m > 59) { printf("Invalid minute; try again.\n"); continue; }
        break;
    }
    while (1) {
        printf("AM or PM (A/P): ");
        if (scanf(" %c", &ampm) != 1) { continue; }
        if (ampm == 'A' || ampm == 'a' || ampm == 'P' || ampm == 'p') break;
        printf("Enter A or P.\n");
    }
    *outH24 = to24Hour(h12, ampm);
    *outM = m;
}

/* read a single char answer y/n safely */
char readYesNo() {
    char c;
    while (1) {
        if (scanf(" %c", &c) == 1) {
            if (c == 'y' || c == 'Y' || c == 'n' || c == 'N') return c;
        }
        printf("Please enter y or n: ");
    }
}

/* ---------------------------------------------------------------------
   Classes: add, print, clear
   --------------------------------------------------------------------- */
void addClasses() {
    printf("\n--- Add Classes ---\n");
    char yn;
    while (numClasses < 20) {
        printf("\nClass #%d\n", numClasses + 1);

        char dep[64];
        printf("Department (single token, e.g. CS): ");
        scanf("%63s", dep);
        s.classes[numClasses].dep = strdup(dep);

        printf("Course number: ");
        scanf("%d", &s.classes[numClasses].courseNum);

        printf("Section number: ");
        scanf("%d", &s.classes[numClasses].section);

        for (int d = 0; d < 7; d++) s.classes[numClasses].days[d] = false;
        printf("Enter days (0=Sun .. 6=Sat). Enter -1 when done:\n");
        int day;
        while (1) {
            if (scanf("%d", &day) != 1) { scanf("%*s"); continue; }
            if (day == -1) break;
            if (day >= 0 && day <= 6) s.classes[numClasses].days[day] = true;
            else printf("Invalid day (0..6). Try again.\n");
        }

        printf("Start time:\n");
        readAMPMTime(" Start hour", &s.classes[numClasses].ts.startHTime, &s.classes[numClasses].ts.startMTime);
        printf("End time:\n");
        readAMPMTime(" End hour", &s.classes[numClasses].ts.endHTime, &s.classes[numClasses].ts.endMTime);

        printf("Is this class on campus? (y/n): ");
        yn = readYesNo();
        s.classes[numClasses].onCampus = (yn == 'y' || yn == 'Y');

        numClasses++;

        if (numClasses >= 20) { printf("Reached maximum classes (20).\n"); break; }

        printf("Add another class? (y/n): ");
        yn = readYesNo();
        if (yn == 'n' || yn == 'N') break;
    }
}

void clearClasses() {
    for (int i = 0; i < numClasses; i++) {
        free((void*)s.classes[i].dep); // free strdup'd strings
        s.classes[i] = emptyClass;
    }
    numClasses = 0;
}

void printClasses() {
    if (numClasses == 0) {
        printf("No classes.\n");
        return;
    }
    for (int i = 0; i < numClasses; i++) {
        printf("\nClass %d:\n", i+1);
        printf("  Dept: %s  Course: %d  Section: %d\n", s.classes[i].dep, s.classes[i].courseNum, s.classes[i].section);
        printf("  Days: ");
        for (int d = 0; d < 7; d++) if (s.classes[i].days[d]) printf("%s ", DayNames[d]);
        printf("\n");
        printf("  Time: "); print12Hour(s.classes[i].ts.startHTime, s.classes[i].ts.startMTime);
        printf("  - "); print12Hour(s.classes[i].ts.endHTime, s.classes[i].ts.endMTime);
        printf("\n  On campus: %s\n", s.classes[i].onCampus ? "Yes" : "No");
    }
}

/* ---------------------------------------------------------------------
   Events: add, print, clear
   --------------------------------------------------------------------- */
void addEvents() {
    printf("\n--- Add Events ---\n");
    char yn;
    while (numEvents < 20) {
        printf("\nEvent #%d\n", numEvents + 1);

        printf("On campus? (y/n): ");
        yn = readYesNo();
        s.events[numEvents].onCampus = (yn == 'y' || yn == 'Y');

        printf("Day (0=Sun .. 6=Sat): ");
        int d;
        while (1) {
            if (scanf("%d", &d) == 1 && d >= 0 && d <= 6) break;
            printf("Enter a day 0..6: ");
            scanf("%*s");
        }
        s.events[numEvents].d = (enum Day)d;

        printf("Start time:\n");
        readAMPMTime(" Start hour", &s.events[numEvents].ts.startHTime, &s.events[numEvents].ts.startMTime);
        printf("End time:\n");
        readAMPMTime(" End hour", &s.events[numEvents].ts.endHTime, &s.events[numEvents].ts.endMTime);

        numEvents++;

        if (numEvents >= 20) { printf("Reached maximum events (20).\n"); break; }

        printf("Add another event? (y/n): ");
        yn = readYesNo();
        if (yn == 'n' || yn == 'N') break;
    }
}

void clearEvents() {
    for (int i = 0; i < numEvents; i++) s.events[i] = emptyEvent;
    numEvents = 0;
}

void printEvents() {
    if (numEvents == 0) {
        printf("No events.\n");
        return;
    }
    for (int i = 0; i < numEvents; i++) {
        printf("\nEvent %d:\n", i+1);
        printf("  Day: %s\n", DayNames[s.events[i].d]);
        printf("  Time: "); print12Hour(s.events[i].ts.startHTime, s.events[i].ts.startMTime);
        printf(" - "); print12Hour(s.events[i].ts.endHTime, s.events[i].ts.endMTime);
        printf("\n  On campus: %s\n", s.events[i].onCampus ? "Yes" : "No");
    }
}

/* ---------------------------------------------------------------------
   Commute input
   --------------------------------------------------------------------- */
void editCommuteTime() {
    printf("\n--- Commute Time ---\n");
    printf("Enter commute time (how long it takes to get to campus) as AM/PM-free hours/mins\n");
    while (1) {
        printf("Hours (0 or more): ");
        if (scanf("%d", &s.commuteTime.startHTime) == 1 && s.commuteTime.startHTime >= 0) break;
        printf("Invalid. Try again.\n");
        scanf("%*s");
    }
    while (1) {
        printf("Minutes (0..59): ");
        if (scanf("%d", &s.commuteTime.startMTime) == 1 && s.commuteTime.startMTime >= 0 && s.commuteTime.startMTime < 60) break;
        printf("Invalid. Try again.\n");
        scanf("%*s");
    }
    printf("Commute set to %d hours %d minutes.\n", s.commuteTime.startHTime, s.commuteTime.startMTime);
}

/* ---------------------------------------------------------------------
   Departure/Arrival calculation per day (based on on-campus classes/events)
   --------------------------------------------------------------------- */
void printDailyDepartures() {
    printf("\n--- Departure / Arrival (per day) ---\n");
    int commuteMins = toMinutes(s.commuteTime.startHTime, s.commuteTime.startMTime);

    for (int d = 0; d < 7; d++) {
        int earliest = 24*60; // large
        int latest = -1;
        bool hasOnCampus = false;

        // classes
        for (int i = 0; i < numClasses; i++) {
            if (s.classes[i].onCampus && s.classes[i].days[d]) {
                int start = toMinutes(s.classes[i].ts.startHTime, s.classes[i].ts.startMTime);
                int end   = toMinutes(s.classes[i].ts.endHTime, s.classes[i].ts.endMTime);
                if (start < earliest) earliest = start;
                if (end > latest) latest = end;
                hasOnCampus = true;
            }
        }

        // events
        for (int i = 0; i < numEvents; i++) {
            if (s.events[i].onCampus && s.events[i].d == d) {
                int start = toMinutes(s.events[i].ts.startHTime, s.events[i].ts.startMTime);
                int end   = toMinutes(s.events[i].ts.endHTime, s.events[i].ts.endMTime);
                if (start < earliest) earliest = start;
                if (end > latest) latest = end;
                hasOnCampus = true;
            }
        }

        if (!hasOnCampus) {
            printf("%s: No on-campus activity.\n", DayNames[d]);
            continue;
        }

        int dep = earliest - commuteMins;
        int arr = latest + commuteMins;

        if (dep < 0) dep = 0;
        if (arr > 24*60 - 1) arr = 24*60 - 1;

        int depH, depM, arrH, arrM;
        fromMinutes(dep, &depH, &depM);
        fromMinutes(arr, &arrH, &arrM);

        printf("%s: Leave at ", DayNames[d]);
        print12Hour(depH, depM);
        printf(", Arrive home at ");
        print12Hour(arrH, arrM);
        printf("\n");
    }
}

/* ---------------------------------------------------------------------
   Print schedule summary
   --------------------------------------------------------------------- */
void printSchedule() {
    printf("\n================ SCHEDULE ================\n");
    printf("Commute: %d hours %d minutes\n", s.commuteTime.startHTime, s.commuteTime.startMTime);

    printf("\n--- CLASSES ---\n");
    printClasses();

    printf("\n--- EVENTS ---\n");
    printEvents();

    printDailyDepartures();
    printf("==========================================\n");
}

/* ---------------------------------------------------------------------
   File I/O format:
   Line1: commuteH commuteM
   Line2: savedWeekNumber (0..53)  -- week number when file was written
   Line3: numClasses
   then each class:
     dep course section days(7 ints 0/1) startH startM endH endM onCampus
   then:
   numEvents
   for each event:
     day startH startM endH endM onCampus
   --------------------------------------------------------------------- */

int currentWeekNumber() {
    time_t t = time(NULL);
    struct tm lt;
    localtime_r(&t, &lt);
    char buf[8];
    // %U - week number of year, Sunday as first day of week, 00..53
    strftime(buf, sizeof(buf), "%U", &lt);
    return atoi(buf);
}

void saveInfo() {
    FILE *file = fopen("schedule.txt", "w");
    if (!file) {
        printf("Error opening file for writing.\n");
        return;
    }

    fprintf(file, "%d %d\n", s.commuteTime.startHTime, s.commuteTime.startMTime);

    int week = currentWeekNumber();
    fprintf(file, "%d\n", week);

    fprintf(file, "%d\n", numClasses);
    for (int i = 0; i < numClasses; i++) {
        // write department as single token
        fprintf(file, "%s %d %d ", s.classes[i].dep, s.classes[i].courseNum, s.classes[i].section);
        for (int d = 0; d < 7; d++) fprintf(file, "%d ", s.classes[i].days[d] ? 1 : 0);
        fprintf(file, "%d %d %d %d %d\n",
            s.classes[i].ts.startHTime,
            s.classes[i].ts.startMTime,
            s.classes[i].ts.endHTime,
            s.classes[i].ts.endMTime,
            s.classes[i].onCampus ? 1 : 0
        );
    }

    fprintf(file, "%d\n", numEvents);
    for (int i = 0; i < numEvents; i++) {
        fprintf(file, "%d %d %d %d %d %d\n",
            s.events[i].d,
            s.events[i].ts.startHTime,
            s.events[i].ts.startMTime,
            s.events[i].ts.endHTime,
            s.events[i].ts.endMTime,
            s.events[i].onCampus ? 1 : 0
        );
    }

    fclose(file);
    printf("Saved schedule (week %d) to schedule.txt\n", week);
}

void grabScheduleFromFile() {
    FILE *file = fopen("schedule.txt", "r");
    if (!file) {
        printf("No schedule file found.\n");
        return;
    }

    int commuteH, commuteM;
    if (fscanf(file, "%d %d\n", &commuteH, &commuteM) != 2) { fclose(file); printf("Bad file.\n"); return; }
    s.commuteTime.startHTime = commuteH;
    s.commuteTime.startMTime = commuteM;

    int savedWeek;
    if (fscanf(file, "%d\n", &savedWeek) != 1) { fclose(file); printf("Bad file (week).\n"); return; }

    int curWeek = currentWeekNumber();
    bool weekMatches = (savedWeek == curWeek);

    // classes
    if (fscanf(file, "%d\n", &numClasses) != 1) { fclose(file); printf("Bad file (numClasses).\n"); return; }
    for (int i = 0; i < numClasses && i < 20; i++) {
        char dep[64];
        int course, section;
        if (fscanf(file, "%63s %d %d", dep, &course, &section) != 3) { printf("Bad class entry.\n"); break; }
        s.classes[i].dep = strdup(dep);
        s.classes[i].courseNum = course;
        s.classes[i].section = section;
        for (int d = 0; d < 7; d++) {
            int flag;
            fscanf(file, "%d", &flag);
            s.classes[i].days[d] = (flag != 0);
        }
        int sh, sm, eh, em, oncamp;
        fscanf(file, "%d %d %d %d %d\n", &sh, &sm, &eh, &em, &oncamp);
        s.classes[i].ts.startHTime = sh;
        s.classes[i].ts.startMTime = sm;
        s.classes[i].ts.endHTime = eh;
        s.classes[i].ts.endMTime = em;
        s.classes[i].onCampus = (oncamp != 0);
    }

    // events: read count, then either load or discard depending on weekMatches
    int fileNumEvents = 0;
    if (fscanf(file, "%d\n", &fileNumEvents) != 1) { fclose(file); printf("Bad file (numEvents).\n"); return; }

    if (!weekMatches) {
        // discard event entries
        for (int i = 0; i < fileNumEvents; i++) {
            int a,b,c,d,e,fv;
            if (fscanf(file, "%d %d %d %d %d %d\n", &a,&b,&c,&d,&e,&fv) != 6) break;
        }
        numEvents = 0;
        printf("Events reset for new week (saved week %d, current week %d).\n", savedWeek, curWeek);
    } else {
        numEvents = 0;
        for (int i = 0; i < fileNumEvents && i < 20; i++) {
            int day, sh, sm, eh, em, oncamp;
            if (fscanf(file, "%d %d %d %d %d %d\n", &day, &sh, &sm, &eh, &em, &oncamp) != 6) break;
            s.events[numEvents].d = (enum Day)day;
            s.events[numEvents].ts.startHTime = sh;
            s.events[numEvents].ts.startMTime = sm;
            s.events[numEvents].ts.endHTime = eh;
            s.events[numEvents].ts.endMTime = em;
            s.events[numEvents].onCampus = (oncamp != 0);
            numEvents++;
        }
        printf("Loaded %d events (week %d matches current week %d).\n", numEvents, savedWeek, curWeek);
    }

    fclose(file);
}

/* ---------------------------------------------------------------------
   Initialization: ask about adding events optionally
   --------------------------------------------------------------------- */
void initializeSchedule() {
    printf("Initialize schedule\n");
    editCommuteTime();

    printf("Do you want to add classes now? (y/n): ");
    if (readYesNo() == 'y') addClasses();

    printf("Do you want to add events now? (y/n): ");
    if (readYesNo() == 'y') addEvents();
}

/* ---------------------------------------------------------------------
   Main menu and program start
   --------------------------------------------------------------------- */
void mainMenu() {
    while (1) {
        printf("\n--- MAIN MENU ---\n");
        printf("1. Print Schedule\n");
        printf("2. Add Classes\n");
        printf("3. Add Events\n");
        printf("4. Clear Classes\n");
        printf("5. Clear Events\n");
        printf("6. Edit Commute Time\n");
        printf("7. Save & Exit\n");
        printf("8. Exit (no save)\n");
        printf("Select: ");
        int choice;
        if (scanf("%d", &choice) != 1) { scanf("%*s"); continue; }
        switch (choice) {
            case 1: printSchedule(); break;
            case 2: addClasses(); break;
            case 3: addEvents(); break;
            case 4: clearClasses(); break;
            case 5: clearEvents(); break;
            case 6: editCommuteTime(); break;
            case 7: saveInfo(); exit(0); break;
            case 8: exit(0); break;
            default: printf("Invalid.\n"); break;
        }
    }
}

int main(void) {
    // if schedule exists, load it (and reset events automatically if week changed)
    if (access("schedule.txt", F_OK) == 0) {
        grabScheduleFromFile();
    } else {
        initializeSchedule();
    }

    mainMenu();
    return 0;
}