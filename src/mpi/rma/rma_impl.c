/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

int MPIR_Win_get_name_impl(MPIR_Win * win_ptr, char *win_name, int *resultlen)
{
    MPL_strncpy(win_name, win_ptr->name, MPI_MAX_OBJECT_NAME);
    *resultlen = (int) strlen(win_name);

    return MPI_SUCCESS;
}

int MPIR_Win_set_name_impl(MPIR_Win * win_ptr, const char *win_name)
{
    MPL_strncpy(win_ptr->name, win_name, MPI_MAX_OBJECT_NAME);

    return MPI_SUCCESS;
}
