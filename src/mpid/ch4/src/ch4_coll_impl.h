/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#ifndef CH4_COLL_IMPL_H_INCLUDED
#define CH4_COLL_IMPL_H_INCLUDED

#include "ch4_csel_container.h"

MPL_STATIC_INLINE_PREFIX int MPIDI_Barrier_intra_composition_alpha(MPIR_Comm * comm,
                                                                   MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int coll_ret = MPI_SUCCESS;

    /* do the intranode barrier on all nodes */
    if (comm->node_comm != NULL) {
#ifndef MPIDI_CH4_DIRECT_NETMOD
        coll_ret = MPIDI_SHM_mpi_barrier(comm->node_comm, errflag);
        if (coll_ret)
            MPIR_ERR_ADD(mpi_errno, coll_ret);
#else
        coll_ret = MPIDI_NM_mpi_barrier(comm->node_comm, errflag);
        if (coll_ret)
            MPIR_ERR_ADD(mpi_errno, coll_ret);
#endif /* MPIDI_CH4_DIRECT_NETMOD */
    }

    /* do the barrier across roots of all nodes */
    if (comm->node_roots_comm != NULL) {
        coll_ret = MPIDI_NM_mpi_barrier(comm->node_roots_comm, errflag);
        if (coll_ret)
            MPIR_ERR_ADD(mpi_errno, coll_ret);
    }

    /* release the local processes on each node with a 1-byte
     * broadcast (0-byte broadcast just returns without doing
     * anything) */
    if (comm->node_comm != NULL) {
        int i = 0;
#ifndef MPIDI_CH4_DIRECT_NETMOD
        coll_ret = MPIDI_SHM_mpi_bcast(&i, 1, MPI_BYTE, 0, comm->node_comm, errflag);
        if (coll_ret)
            MPIR_ERR_ADD(mpi_errno, coll_ret);
#else
        coll_ret = MPIDI_NM_mpi_bcast(&i, 1, MPI_BYTE, 0, comm->node_comm, errflag);
        if (coll_ret)
            MPIR_ERR_ADD(mpi_errno, coll_ret);
#endif /* MPIDI_CH4_DIRECT_NETMOD */
    }

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Barrier_intra_composition_beta(MPIR_Comm * comm,
                                                                  MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIDI_NM_mpi_barrier(comm, errflag);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Bcast_intra_composition_alpha(void *buffer, MPI_Aint count,
                                                                 MPI_Datatype datatype,
                                                                 int root, MPIR_Comm * comm,
                                                                 MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int coll_ret = MPI_SUCCESS;

#ifdef HAVE_ERROR_CHECKING
    MPI_Status status;
    MPI_Aint nbytes, type_size, recvd_size;
#endif

    if (comm->node_roots_comm == NULL && comm->rank == root) {
        coll_ret = MPIC_Send(buffer, count, datatype, 0, MPIR_BCAST_TAG, comm->node_comm, errflag);
        if (coll_ret) {
            *errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(coll_ret) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(coll_ret, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno, coll_ret);
        }
    }

    if (comm->node_roots_comm != NULL && comm->rank != root &&
        MPIR_Get_intranode_rank(comm, root) != -1) {
#ifndef HAVE_ERROR_CHECKING
        coll_ret =
            MPIC_Recv(buffer, count, datatype, MPIR_Get_intranode_rank(comm, root), MPIR_BCAST_TAG,
                      comm->node_comm, MPI_STATUS_IGNORE, errflag);
        if (coll_ret) {
            *errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(coll_ret) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(coll_ret, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno, coll_ret);
        }
#else
        coll_ret =
            MPIC_Recv(buffer, count, datatype, MPIR_Get_intranode_rank(comm, root), MPIR_BCAST_TAG,
                      comm->node_comm, &status, errflag);
        if (coll_ret) {
            *errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(coll_ret) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(coll_ret, *errflag, "**fail");
            MPIR_ERR_ADD(mpi_errno, coll_ret);
        }

        MPIR_Datatype_get_size_macro(datatype, type_size);
        nbytes = type_size * count;
        /* check that we received as much as we expected */
        MPIR_Get_count_impl(&status, MPI_BYTE, &recvd_size);
        if (recvd_size != nbytes) {
            if (*errflag == MPIR_ERR_NONE)
                *errflag = MPIR_ERR_OTHER;
            MPIR_ERR_SET2(coll_ret, MPI_ERR_OTHER,
                          "**collective_size_mismatch",
                          "**collective_size_mismatch %d %d", recvd_size, nbytes);
            MPIR_ERR_ADD(mpi_errno, coll_ret);
        }
#endif
    }

    if (comm->node_roots_comm != NULL) {
        coll_ret =
            MPIDI_NM_mpi_bcast(buffer, count, datatype, MPIR_Get_internode_rank(comm, root),
                               comm->node_roots_comm, errflag);
        if (coll_ret)
            MPIR_ERR_ADD(mpi_errno, coll_ret);
    }
    if (comm->node_comm != NULL) {
#ifndef MPIDI_CH4_DIRECT_NETMOD
        coll_ret = MPIDI_SHM_mpi_bcast(buffer, count, datatype, 0, comm->node_comm, errflag);
        if (coll_ret)
            MPIR_ERR_ADD(mpi_errno, coll_ret);
#else
        coll_ret = MPIDI_NM_mpi_bcast(buffer, count, datatype, 0, comm->node_comm, errflag);
        if (coll_ret)
            MPIR_ERR_ADD(mpi_errno, coll_ret);
#endif /* MPIDI_CH4_DIRECT_NETMOD */
    }

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Bcast_intra_composition_beta(void *buffer, MPI_Aint count,
                                                                MPI_Datatype datatype,
                                                                int root, MPIR_Comm * comm,
                                                                MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int coll_ret = MPI_SUCCESS;

    if (comm->node_comm != NULL && MPIR_Get_intranode_rank(comm, root) > 0) {
#ifndef MPIDI_CH4_DIRECT_NETMOD
        coll_ret =
            MPIDI_SHM_mpi_bcast(buffer, count, datatype, MPIR_Get_intranode_rank(comm, root),
                                comm->node_comm, errflag);
        if (coll_ret)
            MPIR_ERR_ADD(mpi_errno, coll_ret);
#else
        coll_ret =
            MPIDI_NM_mpi_bcast(buffer, count, datatype, MPIR_Get_intranode_rank(comm, root),
                               comm->node_comm, errflag);
        if (coll_ret)
            MPIR_ERR_ADD(mpi_errno, coll_ret);
#endif /* MPIDI_CH4_DIRECT_NETMOD */
    }
    if (comm->node_roots_comm != NULL) {
        coll_ret =
            MPIDI_NM_mpi_bcast(buffer, count, datatype, MPIR_Get_internode_rank(comm, root),
                               comm->node_roots_comm, errflag);
        if (coll_ret)
            MPIR_ERR_ADD(mpi_errno, coll_ret);
    }
    if (comm->node_comm != NULL && MPIR_Get_intranode_rank(comm, root) <= 0) {
#ifndef MPIDI_CH4_DIRECT_NETMOD
        coll_ret = MPIDI_SHM_mpi_bcast(buffer, count, datatype, 0, comm->node_comm, errflag);
        if (coll_ret)
            MPIR_ERR_ADD(mpi_errno, coll_ret);
#else
        coll_ret = MPIDI_NM_mpi_bcast(buffer, count, datatype, 0, comm->node_comm, errflag);
        if (coll_ret)
            MPIR_ERR_ADD(mpi_errno, coll_ret);
#endif /* MPIDI_CH4_DIRECT_NETMOD */
    }

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Bcast_intra_composition_gamma(void *buffer, MPI_Aint count,
                                                                 MPI_Datatype datatype,
                                                                 int root, MPIR_Comm * comm,
                                                                 MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIDI_NM_mpi_bcast(buffer, count, datatype, root, comm, errflag);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Allreduce_intra_composition_alpha(const void *sendbuf,
                                                                     void *recvbuf, MPI_Aint count,
                                                                     MPI_Datatype datatype,
                                                                     MPI_Op op,
                                                                     MPIR_Comm * comm,
                                                                     MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int coll_ret = MPI_SUCCESS;

    if (comm->node_comm != NULL) {
        if ((sendbuf == MPI_IN_PLACE) && (comm->node_comm->rank != 0)) {
#ifndef MPIDI_CH4_DIRECT_NETMOD
            coll_ret =
                MPIDI_SHM_mpi_reduce(recvbuf, NULL, count, datatype, op, 0, comm->node_comm,
                                     errflag);
            if (coll_ret)
                MPIR_ERR_ADD(mpi_errno, coll_ret);
#else
            coll_ret =
                MPIDI_NM_mpi_reduce(recvbuf, NULL, count, datatype, op, 0, comm->node_comm,
                                    errflag);
            if (coll_ret)
                MPIR_ERR_ADD(mpi_errno, coll_ret);
#endif /* MPIDI_CH4_DIRECT_NETMOD */
        } else {
#ifndef MPIDI_CH4_DIRECT_NETMOD
            coll_ret =
                MPIDI_SHM_mpi_reduce(sendbuf, recvbuf, count, datatype, op, 0, comm->node_comm,
                                     errflag);
            if (coll_ret)
                MPIR_ERR_ADD(mpi_errno, coll_ret);
#else
            coll_ret =
                MPIDI_NM_mpi_reduce(sendbuf, recvbuf, count, datatype, op, 0, comm->node_comm,
                                    errflag);
            if (coll_ret)
                MPIR_ERR_ADD(mpi_errno, coll_ret);
#endif /* MPIDI_CH4_DIRECT_NETMOD */
        }
    } else {
        if (sendbuf != MPI_IN_PLACE) {
            coll_ret = MPIR_Localcopy(sendbuf, count, datatype, recvbuf, count, datatype);
            if (coll_ret)
                MPIR_ERR_ADD(mpi_errno, coll_ret);
        }
    }

    if (comm->node_roots_comm != NULL) {
        coll_ret =
            MPIDI_NM_mpi_allreduce(MPI_IN_PLACE, recvbuf, count, datatype, op,
                                   comm->node_roots_comm, errflag);
        if (coll_ret)
            MPIR_ERR_ADD(mpi_errno, coll_ret);
    }

    if (comm->node_comm != NULL) {
#ifndef MPIDI_CH4_DIRECT_NETMOD
        coll_ret = MPIDI_SHM_mpi_bcast(recvbuf, count, datatype, 0, comm->node_comm, errflag);
        if (coll_ret)
            MPIR_ERR_ADD(mpi_errno, coll_ret);
#else
        coll_ret = MPIDI_NM_mpi_bcast(recvbuf, count, datatype, 0, comm->node_comm, errflag);
        if (coll_ret)
            MPIR_ERR_ADD(mpi_errno, coll_ret);
#endif
    }

    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Allreduce_intra_composition_beta(const void *sendbuf,
                                                                    void *recvbuf, MPI_Aint count,
                                                                    MPI_Datatype datatype,
                                                                    MPI_Op op,
                                                                    MPIR_Comm * comm,
                                                                    MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIDI_NM_mpi_allreduce(sendbuf, recvbuf, count, datatype, op, comm, errflag);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Allreduce_intra_composition_gamma(const void *sendbuf,
                                                                     void *recvbuf, MPI_Aint count,
                                                                     MPI_Datatype datatype,
                                                                     MPI_Op op,
                                                                     MPIR_Comm * comm,
                                                                     MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

#ifndef MPIDI_CH4_DIRECT_NETMOD
    mpi_errno = MPIDI_SHM_mpi_allreduce(sendbuf, recvbuf, count, datatype, op, comm, errflag);
#else
    mpi_errno = MPIDI_NM_mpi_allreduce(sendbuf, recvbuf, count, datatype, op, comm, errflag);
#endif
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Reduce_intra_composition_alpha(const void *sendbuf,
                                                                  void *recvbuf, MPI_Aint count,
                                                                  MPI_Datatype datatype,
                                                                  MPI_Op op, int root,
                                                                  MPIR_Comm * comm,
                                                                  MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int coll_ret = MPI_SUCCESS;
    MPI_Aint true_lb = 0;
    MPI_Aint true_extent = 0;
    MPI_Aint extent = 0;
    const void *inter_sendbuf;
    void *ori_recvbuf = recvbuf;

    MPIR_CHKLMEM_DECL(1);

    /* Create a temporary buffer on local roots of all nodes,
     * except for root if it is also a local root */
    if (comm->node_roots_comm != NULL && comm->rank != root) {

        MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
        MPIR_Datatype_get_extent_macro(datatype, extent);

        MPIR_CHKLMEM_MALLOC(recvbuf, void *, count * (MPL_MAX(extent, true_extent)),
                            mpi_errno, "temporary buffer", MPL_MEM_BUFFER);
        /* adjust for potential negative lower bound in datatype */
        recvbuf = (void *) ((char *) recvbuf - true_lb);
    }

    /* intranode reduce on all nodes */
    if (comm->node_comm != NULL) {
#ifndef MPIDI_CH4_DIRECT_NETMOD
        mpi_errno = MPIDI_SHM_mpi_reduce(sendbuf, recvbuf, count, datatype, op, 0, comm->node_comm,
                                         errflag);
#else
        mpi_errno = MPIDI_NM_mpi_reduce(sendbuf, recvbuf, count, datatype, op, 0, comm->node_comm,
                                        errflag);
#endif /* MPIDI_CH4_DIRECT_NETMOD */

        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(coll_ret, mpi_errno);
        }
        /* recvbuf becomes the sendbuf for internode reduce */
        inter_sendbuf = recvbuf;
    } else {
        inter_sendbuf = (sendbuf == MPI_IN_PLACE) ? recvbuf : sendbuf;
    }

    /* internode reduce with rank 0 in node_roots_comm as the root */
    if (comm->node_roots_comm != NULL) {
        mpi_errno =
            MPIDI_NM_mpi_reduce(comm->node_roots_comm->rank == 0 ? MPI_IN_PLACE : inter_sendbuf,
                                recvbuf, count, datatype, op, 0, comm->node_roots_comm, errflag);

        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(coll_ret, mpi_errno);
        }
    }

    /* Send data to root via point-to-point message if root is not rank 0 in comm */
    if (root != 0) {
        if (comm->rank == 0) {
            MPIC_Send(recvbuf, count, datatype, root, MPIR_REDUCE_TAG, comm, errflag);
        } else if (comm->rank == root) {
            MPIC_Recv(ori_recvbuf, count, datatype, 0, MPIR_REDUCE_TAG, comm, MPI_STATUS_IGNORE,
                      errflag);
        }
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Reduce_intra_composition_beta(const void *sendbuf,
                                                                 void *recvbuf, MPI_Aint count,
                                                                 MPI_Datatype datatype,
                                                                 MPI_Op op, int root,
                                                                 MPIR_Comm * comm,
                                                                 MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int coll_ret = MPI_SUCCESS;
    MPI_Aint true_lb = 0;
    MPI_Aint true_extent = 0;
    MPI_Aint extent = 0;

    MPIR_CHKLMEM_DECL(1);

    void *tmp_buf = NULL;

    /* Create a temporary buffer on local roots of all nodes */
    if (comm->node_roots_comm != NULL) {

        MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);
        MPIR_Datatype_get_extent_macro(datatype, extent);

        MPIR_CHKLMEM_MALLOC(tmp_buf, void *, count * (MPL_MAX(extent, true_extent)),
                            mpi_errno, "temporary buffer", MPL_MEM_BUFFER);
        /* adjust for potential negative lower bound in datatype */
        tmp_buf = (void *) ((char *) tmp_buf - true_lb);
    }

    /* do the intranode reduce on all nodes other than the root's node */
    if (comm->node_comm != NULL && MPIR_Get_intranode_rank(comm, root) == -1) {
#ifndef MPIDI_CH4_DIRECT_NETMOD
        mpi_errno =
            MPIDI_SHM_mpi_reduce(sendbuf, tmp_buf, count, datatype, op, 0, comm->node_comm,
                                 errflag);
#else
        mpi_errno =
            MPIDI_NM_mpi_reduce(sendbuf, tmp_buf, count, datatype, op, 0, comm->node_comm, errflag);
#endif /* MPIDI_CH4_DIRECT_NETMOD */
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(coll_ret, mpi_errno);
        }
    }

    /* do the internode reduce to the root's node */
    if (comm->node_roots_comm != NULL) {
        if (comm->node_roots_comm->rank != MPIR_Get_internode_rank(comm, root)) {
            /* I am not on root's node.  Use tmp_buf if we
             * participated in the first reduce, otherwise use sendbuf */
            const void *buf = (comm->node_comm == NULL ? sendbuf : tmp_buf);
            mpi_errno =
                MPIDI_NM_mpi_reduce(buf, NULL, count, datatype,
                                    op, MPIR_Get_internode_rank(comm, root),
                                    comm->node_roots_comm, errflag);
            if (mpi_errno) {
                /* for communication errors, just record the error but continue */
                *errflag =
                    MPIX_ERR_PROC_FAILED ==
                    MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                MPIR_ERR_ADD(coll_ret, mpi_errno);
            }
        } else {        /* I am on root's node. I have not participated in the earlier reduce. */
            if (comm->rank != root) {
                /* I am not the root though. I don't have a valid recvbuf.
                 * Use tmp_buf as recvbuf. */
                mpi_errno =
                    MPIDI_NM_mpi_reduce(sendbuf, tmp_buf, count, datatype,
                                        op, MPIR_Get_internode_rank(comm, root),
                                        comm->node_roots_comm, errflag);

                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag =
                        MPIX_ERR_PROC_FAILED ==
                        MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(coll_ret, mpi_errno);
                }

                /* point sendbuf at tmp_buf to make final intranode reduce easy */
                sendbuf = tmp_buf;
            } else {
                /* I am the root. in_place is automatically handled. */
                mpi_errno =
                    MPIDI_NM_mpi_reduce(sendbuf, recvbuf, count, datatype,
                                        op, MPIR_Get_internode_rank(comm, root),
                                        comm->node_roots_comm, errflag);
                if (mpi_errno) {
                    /* for communication errors, just record the error but continue */
                    *errflag =
                        MPIX_ERR_PROC_FAILED ==
                        MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
                    MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
                    MPIR_ERR_ADD(coll_ret, mpi_errno);
                }

                /* set sendbuf to MPI_IN_PLACE to make final intranode reduce easy. */
                sendbuf = MPI_IN_PLACE;
            }
        }

    }

    /* do the intranode reduce on the root's node */
    if (comm->node_comm != NULL && MPIR_Get_intranode_rank(comm, root) != -1) {
#ifndef MPIDI_CH4_DIRECT_NETMOD
        mpi_errno =
            MPIDI_SHM_mpi_reduce(sendbuf, recvbuf, count, datatype,
                                 op, MPIR_Get_intranode_rank(comm, root), comm->node_comm, errflag);
#else
        mpi_errno =
            MPIDI_NM_mpi_reduce(sendbuf, recvbuf, count, datatype,
                                op, MPIR_Get_intranode_rank(comm, root), comm->node_comm, errflag);
#endif /* MPIDI_CH4_DIRECT_NETMOD */
        if (mpi_errno) {
            /* for communication errors, just record the error but continue */
            *errflag =
                MPIX_ERR_PROC_FAILED ==
                MPIR_ERR_GET_CLASS(mpi_errno) ? MPIR_ERR_PROC_FAILED : MPIR_ERR_OTHER;
            MPIR_ERR_SET(mpi_errno, *errflag, "**fail");
            MPIR_ERR_ADD(coll_ret, mpi_errno);
        }
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;

}


MPL_STATIC_INLINE_PREFIX int MPIDI_Reduce_intra_composition_gamma(const void *sendbuf,
                                                                  void *recvbuf, MPI_Aint count,
                                                                  MPI_Datatype datatype,
                                                                  MPI_Op op, int root,
                                                                  MPIR_Comm * comm,
                                                                  MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIDI_NM_mpi_reduce(sendbuf, recvbuf, count, datatype, op, root, comm, errflag);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Alltoall_intra_composition_beta(const void *sendbuf,
                                                                   MPI_Aint sendcount,
                                                                   MPI_Datatype sendtype,
                                                                   void *recvbuf,
                                                                   MPI_Aint recvcount,
                                                                   MPI_Datatype recvtype,
                                                                   MPIR_Comm * comm_ptr,
                                                                   MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIDI_NM_mpi_alltoall(sendbuf, sendcount, sendtype, recvbuf,
                              recvcount, recvtype, comm_ptr, errflag);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Alltoallv_intra_composition_alpha(const void *sendbuf,
                                                                     const MPI_Aint * sendcounts,
                                                                     const MPI_Aint * sdispls,
                                                                     MPI_Datatype sendtype,
                                                                     void *recvbuf,
                                                                     const MPI_Aint * recvcounts,
                                                                     const MPI_Aint * rdispls,
                                                                     MPI_Datatype recvtype,
                                                                     MPIR_Comm * comm_ptr,
                                                                     MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIDI_NM_mpi_alltoallv(sendbuf, sendcounts, sdispls,
                               sendtype, recvbuf, recvcounts, rdispls, recvtype, comm_ptr, errflag);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Alltoallw_intra_composition_alpha(const void *sendbuf,
                                                                     const MPI_Aint sendcounts[],
                                                                     const MPI_Aint sdispls[],
                                                                     const MPI_Datatype
                                                                     sendtypes[],
                                                                     void *recvbuf,
                                                                     const MPI_Aint recvcounts[],
                                                                     const MPI_Aint rdispls[],
                                                                     const MPI_Datatype
                                                                     recvtypes[],
                                                                     MPIR_Comm * comm_ptr,
                                                                     MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIDI_NM_mpi_alltoallw(sendbuf, sendcounts, sdispls,
                               sendtypes, recvbuf, recvcounts,
                               rdispls, recvtypes, comm_ptr, errflag);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Allgather_intra_composition_beta(const void *sendbuf,
                                                                    MPI_Aint sendcount,
                                                                    MPI_Datatype sendtype,
                                                                    void *recvbuf,
                                                                    MPI_Aint recvcount,
                                                                    MPI_Datatype recvtype,
                                                                    MPIR_Comm * comm_ptr,
                                                                    MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIDI_NM_mpi_allgather(sendbuf, sendcount, sendtype,
                               recvbuf, recvcount, recvtype, comm_ptr, errflag);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Allgatherv_intra_composition_alpha(const void *sendbuf,
                                                                      MPI_Aint sendcount,
                                                                      MPI_Datatype sendtype,
                                                                      void *recvbuf,
                                                                      const MPI_Aint * recvcounts,
                                                                      const MPI_Aint * displs,
                                                                      MPI_Datatype recvtype,
                                                                      MPIR_Comm * comm_ptr,
                                                                      MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIDI_NM_mpi_allgatherv(sendbuf, sendcount, sendtype,
                                recvbuf, recvcounts, displs, recvtype, comm_ptr, errflag);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Gather_intra_composition_alpha(const void *sendbuf,
                                                                  MPI_Aint sendcount,
                                                                  MPI_Datatype sendtype,
                                                                  void *recvbuf, MPI_Aint recvcount,
                                                                  MPI_Datatype recvtype,
                                                                  int root, MPIR_Comm * comm,
                                                                  MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIDI_NM_mpi_gather(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                            recvtype, root, comm, errflag);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Gatherv_intra_composition_alpha(const void *sendbuf,
                                                                   MPI_Aint sendcount,
                                                                   MPI_Datatype sendtype,
                                                                   void *recvbuf,
                                                                   const MPI_Aint * recvcounts,
                                                                   const MPI_Aint * displs,
                                                                   MPI_Datatype recvtype,
                                                                   int root, MPIR_Comm * comm,
                                                                   MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIDI_NM_mpi_gatherv(sendbuf, sendcount, sendtype, recvbuf, recvcounts,
                             displs, recvtype, root, comm, errflag);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Scatter_intra_composition_alpha(const void *sendbuf,
                                                                   MPI_Aint sendcount,
                                                                   MPI_Datatype sendtype,
                                                                   void *recvbuf,
                                                                   MPI_Aint recvcount,
                                                                   MPI_Datatype recvtype,
                                                                   int root, MPIR_Comm * comm,
                                                                   MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIDI_NM_mpi_scatter(sendbuf, sendcount, sendtype, recvbuf, recvcount,
                             recvtype, root, comm, errflag);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Scatterv_intra_composition_alpha(const void *sendbuf,
                                                                    const MPI_Aint * sendcounts,
                                                                    const MPI_Aint * displs,
                                                                    MPI_Datatype sendtype,
                                                                    void *recvbuf,
                                                                    MPI_Aint recvcount,
                                                                    MPI_Datatype recvtype,
                                                                    int root, MPIR_Comm * comm,
                                                                    MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIDI_NM_mpi_scatterv(sendbuf, sendcounts, displs, sendtype, recvbuf,
                              recvcount, recvtype, root, comm, errflag);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Reduce_scatter_intra_composition_alpha(const void *sendbuf,
                                                                          void *recvbuf,
                                                                          const MPI_Aint
                                                                          recvcounts[],
                                                                          MPI_Datatype
                                                                          datatype, MPI_Op op,
                                                                          MPIR_Comm * comm_ptr,
                                                                          MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIDI_NM_mpi_reduce_scatter(sendbuf, recvbuf, recvcounts, datatype, op, comm_ptr, errflag);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Reduce_scatter_block_intra_composition_alpha(const void
                                                                                *sendbuf,
                                                                                void *recvbuf,
                                                                                MPI_Aint recvcount,
                                                                                MPI_Datatype
                                                                                datatype,
                                                                                MPI_Op op,
                                                                                MPIR_Comm *
                                                                                comm_ptr,
                                                                                MPIR_Errflag_t
                                                                                * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno =
        MPIDI_NM_mpi_reduce_scatter_block(sendbuf, recvbuf, recvcount, datatype,
                                          op, comm_ptr, errflag);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Scan_intra_composition_alpha(const void *sendbuf,
                                                                void *recvbuf,
                                                                MPI_Aint count,
                                                                MPI_Datatype datatype,
                                                                MPI_Op op,
                                                                MPIR_Comm * comm_ptr,
                                                                MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    int coll_ret = MPI_SUCCESS;
    int rank = comm_ptr->rank;
    MPI_Status status;
    void *tempbuf = NULL;
    void *localfulldata = NULL;
    void *prefulldata = NULL;
    MPI_Aint true_lb = 0;
    MPI_Aint true_extent = 0;
    MPI_Aint extent = 0;
    int noneed = 1;             /* noneed=1 means no need to bcast tempbuf and
                                 * reduce tempbuf & recvbuf */
    MPIR_CHKLMEM_DECL(3);


    MPIR_Type_get_true_extent_impl(datatype, &true_lb, &true_extent);

    MPIR_Datatype_get_extent_macro(datatype, extent);

    MPIR_CHKLMEM_MALLOC(tempbuf, void *, count * (MPL_MAX(extent, true_extent)),
                        mpi_errno, "temporary buffer", MPL_MEM_BUFFER);
    tempbuf = (void *) ((char *) tempbuf - true_lb);

    /* Create prefulldata and localfulldata on local roots of all nodes */
    if (comm_ptr->node_roots_comm != NULL) {
        MPIR_CHKLMEM_MALLOC(prefulldata, void *, count * (MPL_MAX(extent, true_extent)),
                            mpi_errno, "prefulldata for scan", MPL_MEM_BUFFER);
        prefulldata = (void *) ((char *) prefulldata - true_lb);

        if (comm_ptr->node_comm != NULL) {
            MPIR_CHKLMEM_MALLOC(localfulldata, void *, count * (MPL_MAX(extent, true_extent)),
                                mpi_errno, "localfulldata for scan", MPL_MEM_BUFFER);
            localfulldata = (void *) ((char *) localfulldata - true_lb);
        }
    }

    /* perform intranode scan to get temporary result in recvbuf. if there is only
     * one process, just copy the raw data. */
    if (comm_ptr->node_comm != NULL) {
#ifndef MPIDI_CH4_DIRECT_NETMOD
        coll_ret =
            MPIDI_SHM_mpi_scan(sendbuf, recvbuf, count, datatype, op, comm_ptr->node_comm, errflag);
        if (coll_ret)
            MPIR_ERR_ADD(mpi_errno, coll_ret);
#else
        coll_ret =
            MPIDI_NM_mpi_scan(sendbuf, recvbuf, count, datatype, op, comm_ptr->node_comm, errflag);
        if (coll_ret)
            MPIR_ERR_ADD(mpi_errno, coll_ret);
#endif /* MPIDI_CH4_DIRECT_NETMOD */
    } else if (sendbuf != MPI_IN_PLACE) {
        coll_ret = MPIR_Localcopy(sendbuf, count, datatype, recvbuf, count, datatype);
        if (coll_ret)
            MPIR_ERR_ADD(mpi_errno, coll_ret);
    }
    /* get result from local node's last processor which
     * contains the reduce result of the whole node. Name it as
     * localfulldata. For example, localfulldata from node 1 contains
     * reduced data of rank 1,2,3. */
    if (comm_ptr->node_roots_comm != NULL && comm_ptr->node_comm != NULL) {
        coll_ret = MPIC_Recv(localfulldata, count, datatype,
                             comm_ptr->node_comm->local_size - 1, MPIR_SCAN_TAG,
                             comm_ptr->node_comm, &status, errflag);
        if (coll_ret)
            MPIR_ERR_ADD(mpi_errno, coll_ret);
    } else if (comm_ptr->node_roots_comm == NULL &&
               comm_ptr->node_comm != NULL &&
               MPIR_Get_intranode_rank(comm_ptr, rank) == comm_ptr->node_comm->local_size - 1) {
        coll_ret = MPIC_Send(recvbuf, count, datatype,
                             0, MPIR_SCAN_TAG, comm_ptr->node_comm, errflag);
        if (coll_ret)
            MPIR_ERR_ADD(mpi_errno, coll_ret);
    } else if (comm_ptr->node_roots_comm != NULL) {
        localfulldata = recvbuf;
    }
    /* do scan on localfulldata to prefulldata. for example,
     * prefulldata on rank 4 contains reduce result of ranks
     * 1,2,3,4,5,6. it will be sent to rank 7 which is the main
     * process of node 3. */
    if (comm_ptr->node_roots_comm != NULL) {
        coll_ret =
            MPIDI_NM_mpi_scan(localfulldata, prefulldata, count, datatype,
                              op, comm_ptr->node_roots_comm, errflag);
        if (coll_ret)
            MPIR_ERR_ADD(mpi_errno, coll_ret);

        if (MPIR_Get_internode_rank(comm_ptr, rank) != comm_ptr->node_roots_comm->local_size - 1) {
            coll_ret = MPIC_Send(prefulldata, count, datatype,
                                 MPIR_Get_internode_rank(comm_ptr, rank) + 1,
                                 MPIR_SCAN_TAG, comm_ptr->node_roots_comm, errflag);
            if (coll_ret)
                MPIR_ERR_ADD(mpi_errno, coll_ret);
        }
        if (MPIR_Get_internode_rank(comm_ptr, rank) != 0) {
            coll_ret = MPIC_Recv(tempbuf, count, datatype,
                                 MPIR_Get_internode_rank(comm_ptr, rank) - 1,
                                 MPIR_SCAN_TAG, comm_ptr->node_roots_comm, &status, errflag);
            noneed = 0;
            if (coll_ret)
                MPIR_ERR_ADD(mpi_errno, coll_ret);
        }
    }

    /* now tempbuf contains all the data needed to get the correct
     * scan result. for example, to node 3, it will have reduce result
     * of rank 1,2,3,4,5,6 in tempbuf.
     * then we should broadcast this result in the local node, and
     * reduce it with recvbuf to get final result if necessary. */

    if (comm_ptr->node_comm != NULL) {
#ifndef MPIDI_CH4_DIRECT_NETMOD
        coll_ret = MPIDI_SHM_mpi_bcast(&noneed, 1, MPI_INT, 0, comm_ptr->node_comm, errflag);
        if (coll_ret)
            MPIR_ERR_ADD(mpi_errno, coll_ret);
#else
        coll_ret = MPIDI_NM_mpi_bcast(&noneed, 1, MPI_INT, 0, comm_ptr->node_comm, errflag);
        if (coll_ret)
            MPIR_ERR_ADD(mpi_errno, coll_ret);
#endif /* MPIDI_CH4_DIRECT_NETMOD */
    }

    if (noneed == 0) {
        if (comm_ptr->node_comm != NULL) {
#ifndef MPIDI_CH4_DIRECT_NETMOD
            coll_ret =
                MPIDI_SHM_mpi_bcast(tempbuf, count, datatype, 0, comm_ptr->node_comm, errflag);
            if (coll_ret)
                MPIR_ERR_ADD(mpi_errno, coll_ret);
#else
            coll_ret =
                MPIDI_NM_mpi_bcast(tempbuf, count, datatype, 0, comm_ptr->node_comm, errflag);
            if (coll_ret)
                MPIR_ERR_ADD(mpi_errno, coll_ret);
#endif /* MPIDI_CH4_DIRECT_NETMOD */
        }

        coll_ret = MPIR_Reduce_local(tempbuf, recvbuf, count, datatype, op);
        if (coll_ret)
            MPIR_ERR_ADD(mpi_errno, coll_ret);
    }

  fn_exit:
    MPIR_CHKLMEM_FREEALL();
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Scan_intra_composition_beta(const void *sendbuf,
                                                               void *recvbuf,
                                                               MPI_Aint count,
                                                               MPI_Datatype datatype,
                                                               MPI_Op op,
                                                               MPIR_Comm * comm_ptr,
                                                               MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIDI_NM_mpi_scan(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_Exscan_intra_composition_alpha(const void *sendbuf,
                                                                  void *recvbuf,
                                                                  MPI_Aint count,
                                                                  MPI_Datatype datatype,
                                                                  MPI_Op op,
                                                                  MPIR_Comm * comm_ptr,
                                                                  MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    mpi_errno = MPIDI_NM_mpi_exscan(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

#endif /* CH4_COLL_IMPL_H_INCLUDED */
