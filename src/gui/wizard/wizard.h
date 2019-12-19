// wizard.h

#ifndef _WIZARD_H
#define _WIZARD_H

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

extern void wizard_run_complete(void);
extern void wizard_run_selftest(void);
extern void wizard_run_xyzcalib(void);
extern void wizard_run_xyzcalib_xy(void);
extern void wizard_run_xyzcalib_z(void);
extern void wizard_run_firstlay(void);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // _WIZARD_H
