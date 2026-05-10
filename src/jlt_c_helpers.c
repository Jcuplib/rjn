/*
 * jlt_c_helpers.c
 * Bridge helpers called from jlt_interface.f90.
 * Fortran obtains the filename via INQUIRE(unit, name=) and passes it here
 * as a null-terminated C string.  This avoids relying on fnum() which is
 * not available in all Fortran compiler runtimes (e.g. ifx).
 */

#include "rjn_interface.h"

/* Write the mapping table to the named file (opened fresh, truncated). */
void jlt_write_mapping_table_name(const char* fname)
{
    FILE *fp = fopen(fname, "wb");
    if (!fp) return;
    rjn_write_mapping_table(fp);
    fclose(fp);
}

/* Read the mapping table from the named file. */
void jlt_read_mapping_table_name(const char* fname)
{
    FILE *fp = fopen(fname, "rb");
    if (!fp) return;
    rjn_read_mapping_table(fp);
    fclose(fp);
}
