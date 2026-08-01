#ifndef TIMEPOD_NOTIFY_H
#define TIMEPOD_NOTIFY_H

#ifdef __cplusplus
extern "C" {
#endif

/* Simple notification hook.
 * Qt build may provide UI/desktop notifications; current MVP is a no-op.
 */
void timepod_notify_domain_completed(const char *domain_name);

#ifdef __cplusplus
}
#endif

#endif

