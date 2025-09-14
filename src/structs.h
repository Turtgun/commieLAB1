// structs.h
#pragma once
#include <stdbool.h>

enum Day {
    Sunday,
    Monday,
    Tuesday,
    Wednesday,
    Thursday,
    Friday,
    Saturday
};

typedef struct {
    int startHTime; // stored as 24-hour hour (0-23)
    int startMTime;

    int endHTime;   // stored as 24-hour hour (0-23)
    int endMTime;
} TimeSlot;

typedef struct {
    const char* dep;
    int courseNum;
    int section;

    bool days[7];
    TimeSlot ts;
    bool onCampus;
} Class;

typedef struct {
    enum Day d;
    TimeSlot ts;
    bool onCampus;
} Event;

typedef struct {
    Class classes [20];
    Event events [20];

    TimeSlot commuteTime;
} Schedule;
