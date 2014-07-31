#define DRIVERVERSION	"v4.1.5_7309_patched_20130515"

/*
Patched List:
7345-7344_fix_NULL_pointer_crash_cfg80211_rtw_scan.diff
    1. This issue may happen when wifi resuming and buddy's scanning
7346-7345_usb_read_port_complete_error_handle.diff
    1. Add cases: -EILSEQ, -ETIME, -ECOMM which are similiar with -EPROTO and -EOVERFLOW
7347-7346_fix_register_early_suspend_twice.diff
    1. Register early_suspend only for primary_adapter's pwrpriv
7386-7385_88E_IQK_bug_fix.diff
    1. This problem may cause weak TX power when IQK fails
7411-7410_fix_usb_resume_error_handling_deadlock.diff
    1. Fix deadlock problem of rtw_resume_process() when pm_netdev_open() fails
7441-7440_fix_concurrent_lps_bug_dont_dl_rsvd_page_for_port1.diff
    1. The rsvd page of port0 interface will be overrode and FW will TX the wrong null, QoS null
*/
