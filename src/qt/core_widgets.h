#pragma once

// Only primitive values and borrowed UTF-8 strings cross the toolchain boundary.
struct HiasmQtHost {
    const char *(*property)(void *, const char *);
    void (*event)(void *, const char *, const char *, double, int);
};

struct HiasmQtWidgets {
    int (*start)(const HiasmQtHost *, const char *);
    void (*stop)();
    void (*pump)();
    void *(*create)(void *, const char *, void *);
    void (*show)(void *);
    int (*work)(void *, const char *, const char *, double);
    int (*read)(void *, const char *, const char **, double *);
};

using HiasmQtGetWidgets = const HiasmQtWidgets *(*)(unsigned);
