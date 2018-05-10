#define pr_fmt(fmt) "cs423_mp4: " fmt
#define XATTR_MAX_SIZE 11
#define PATH_LEN 1024

#define DEFAULT_ACCESS (MAY_ACCESS|MAY_OPEN|MAY_NOT_BLOCK)
#define READ_ACCESS (MAY_READ|DEFAULT_ACCESS)
#define WRITE_ACCESS (MAY_WRITE|DEFAULT_ACCESS)
#define RDWR_ACCESS (READ_ACCESS|WRITE_ACCESS)
#define EXEC_ACCESS (MAY_READ|MAY_EXEC|DEFAULT_ACCESS)
#define RD_DIR_ACCESS (EXEC_ACCESS|MAY_CHDIR)
#define RW_DIR_ACCESS (RD_DIR_ACCESS|MAY_WRITE)

#include <linux/lsm_hooks.h>
#include <linux/security.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/cred.h>
#include <linux/dcache.h>
#include <linux/binfmts.h>
#include "mp4_given.h"

/**
 * get_inode_sid - Get the inode mp4 security label id
 *
 * @inode: the input inode
 *
 * @return the inode's security id if found.
 *
 */
static int get_inode_sid(struct inode *inode)
{
        int sid, rc;
        char xattr_value[XATTR_MAX_SIZE];
        struct dentry *de;
        de = d_find_alias(inode);
        if (!inode->i_op->getxattr) {
                /*not support*/
                sid = -1;
                goto out;
        }
        rc = inode->i_op->getxattr(de, XATTR_NAME_MP4, xattr_value, XATTR_MAX_SIZE);
        if (rc <= 0) {
                /*no such attribute*/
                sid = -1;
                goto out;
        }
        sid = __cred_ctx_to_sid(xattr_value);
out:
        if (de) {
                dput(de);
        }
        return sid;
}

/**
 * mp4_bprm_set_creds - Set the credentials for a new task
 *
 * @bprm: The linux binary preparation structure
 *
 * returns 0 on success.
 */
static int mp4_bprm_set_creds(struct linux_binprm *bprm)
{
        struct mp4_security *new_mp4_sec;
        struct inode *inode = file_inode(bprm->file);
        int sid;
        new_mp4_sec = bprm->cred->security;
        if(!new_mp4_sec) {
                pr_err("set_creds to null mp4_security\n");
                return -1;
        }
        sid = get_inode_sid(inode);
        if (sid == MP4_TARGET_SID) {
                new_mp4_sec->mp4_flags = MP4_TARGET_SID;
        }
        return 0;
}

/**
 * mp4_cred_alloc_blank - Allocate a blank mp4 security label
 *
 * @cred: the new credentials
 * @gfp: the atomicity of the memory allocation
 *
 */
static int mp4_cred_alloc_blank(struct cred *cred, gfp_t gfp)
{
        struct mp4_security *mp4_sec;

        mp4_sec = kzalloc(sizeof(struct mp4_security), gfp);
        if (!mp4_sec){
                pr_err("kzalloc failed\n");
                return -ENOMEM;
        }
        mp4_sec->mp4_flags = MP4_NO_ACCESS;
        cred->security = mp4_sec;
        //pr_info("mp4_cred_alloc_blank succeed\n");
        return 0;
}


/**
 * mp4_cred_free - Free a created security label
 *
 * @cred: the credentials struct
 *
 */
static void mp4_cred_free(struct cred *cred)
{
        kfree(cred->security);
        cred->security = NULL;
        //pr_info("mp4_cred_free succeed\n");
}

/**
 * mp4_cred_prepare - Prepare new credentials for modification
 *
 * @new: the new credentials
 * @old: the old credentials
 * @gfp: the atomicity of the memory allocation
 *
 */
static int mp4_cred_prepare(struct cred *new, const struct cred *old,
                            gfp_t gfp)
{
        const struct mp4_security *old_mp4_sec;
        struct mp4_security *mp4_sec;

        old_mp4_sec = old->security;
        if (!old_mp4_sec) {
                /* first task would not have our tag */
                mp4_sec = kmalloc(sizeof(struct mp4_security), gfp);
        } else {
                mp4_sec = kmemdup(old_mp4_sec, sizeof(struct mp4_security), gfp);
        }
        if (!mp4_sec){
                pr_err("kmemdup failed\n");
                return -ENOMEM;
        }
        mp4_sec->mp4_flags = MP4_NO_ACCESS;
        new->security = mp4_sec;
        //pr_info("mp4_cred_prepare succeed\n");
        return 0;
}

/**
 * mp4_inode_init_security - Set the security attribute of a newly created inode
 *
 * @inode: the newly created inode
 * @dir: the containing directory
 * @qstr: unused
 * @name: where to put the attribute name
 * @value: where to put the attribute value
 * @len: where to put the length of the attribute
 *
 * returns 0 if all goes well, -ENOMEM if no memory, -EOPNOTSUPP to skip
 *
 */
static int mp4_inode_init_security(struct inode *inode, struct inode *dir,
                                   const struct qstr *qstr,
                                   const char **name, void **value, size_t *len)
{
        const struct mp4_security *curr_mp4_sec = current_security();
        struct dentry *de;
        int rc = -EOPNOTSUPP;

        if (curr_mp4_sec && curr_mp4_sec->mp4_flags == MP4_TARGET_SID) {
                de = d_find_alias(inode);
                rc = inode->i_op->setxattr(de, XATTR_NAME_MP4,
                                        "read-write", strlen("read-write"), 0);
                dput(de);
        //        pr_info("inode_init_security: target\n");
        //} else {
        //        pr_info("inode_init_security: not target\n");
        }
        return rc;
}

static int check_access(int sid, int mask)
{
        int rc = 0;
        switch (sid) {
                case MP4_NO_ACCESS:
                        //rc = -EACCES;
                        pr_info("NO_ACCESS by target\n");
                        rc = 0;
                        break;
                case MP4_READ_OBJ:
                        if ((mask | READ_ACCESS) == READ_ACCESS) {
                                //pr_info("MP4_READ_OBJ and READ_ACCESS by target\n");
                                rc = 0;
                        } else {
                                //rc = -EACCES;
                                pr_info("MP4_READ_OBJ but not READ_ACCESS by target\n");
                                rc = 0;
                        }
                        break;
                case MP4_READ_WRITE:
                        if ((mask | RDWR_ACCESS) == RDWR_ACCESS) {
                                //pr_info("MP4_READ_WRITE and RDWR_ACCESS by target\n");
                                rc = 0;
                        } else {
                                //rc = -EACCES;
                                pr_info("MP4_READ_WRITE but not RDWR_ACCESS by target\n");
                                rc = 0;
                        }
                        break;
                case MP4_WRITE_OBJ:
                        if ((mask | WRITE_ACCESS) == WRITE_ACCESS) {
                                //pr_info("MP4_WRITE_OBJ and WRITE_ACCESS by target\n");
                                rc = 0;
                        } else {
                                //rc = -EACCES;
                                pr_info("MP4_WRITE_OBJ but not WRITE_ACCESS by target\n");
                                rc = 0;
                        }
                        break;
                case MP4_EXEC_OBJ:
                        if ((mask | EXEC_ACCESS) == EXEC_ACCESS) {
                                //pr_info("MP4_EXEC_OBJ and EXEC_ACCESS by target\n");
                                rc = 0;
                        } else {
                                //rc = -EACCES;
                                pr_info("MP4_EXEC_OBJ but not EXEC_ACCESS by target\n");
                                rc = 0;
                        }
                        break;
                case MP4_READ_DIR:
                        if ((mask | RD_DIR_ACCESS) == RD_DIR_ACCESS) {
                                //pr_info("MP4_READ_DIR and RD_DIR_ACCESS by target\n");
                                rc = 0;
                        } else {
                                //rc = -EACCES;
                                pr_info("MP4_READ_DIR but not RD_DIR_ACCESS by target\n");
                                rc = 0;
                        }
                        break;
                case MP4_RW_DIR:
                        if ((mask | RW_DIR_ACCESS) == RW_DIR_ACCESS) {
                                //pr_info("MP4_READ_DIR and RD_DIR_ACCESS by target\n");
                                rc = 0;
                        } else {
                                //rc = -EACCES;
                                pr_info("MP4_READ_DIR but not RD_DIR_ACCESS by target\n");
                                rc = 0;
                        }
                        break;
                default:
                        /*should not match*/
                        pr_info("should not match\n");
                        rc = 0;
                        break;
        }
        return rc;
}

/**
 * mp4_has_permission - Check if subject has permission to an object
 *
 * @ssid: the subject's security id
 * @osid: the object's security id
 * @mask: the operation mask
 *
 * returns 0 is access granter, -EACCES otherwise
 *
 */
static int mp4_has_permission(struct inode *inode, int ssid, int osid, int mask)
{
        int rc;
        if (ssid != MP4_TARGET_SID) {
                /* not target */
                if (S_ISDIR(inode->i_mode)) {
                        /* allow them full access to directories */
                        //pr_info("Dir, not target, granted\n");
                        return 0;
                } else {
                        /* allow read-only access to files that have been
                           assigned one of our custom labels */
                        if ((mask | READ_ACCESS) == READ_ACCESS) {
                                //pr_info("Read, not target, granted\n");
                                return 0;
                        } else {
                                //rc = -EACCES;
                                pr_info("ssid=%d, osid=%d,Not read, not target, denied\n", ssid, osid);
                                return 0;
                        }
                }
        } else {
                /* target */
                rc = check_access(osid, mask);
        }
        return rc;
}


/**
 * mp4_inode_permission - Check permission for an inode being opened
 *
 * @inode: the inode in question
 * @mask: the access requested
 *
 * This is the important access check hook
 *
 * returns 0 if access is granted, -EACCES otherwise
 *
 */
static int mp4_inode_permission(struct inode *inode, int mask)
{
        const struct mp4_security *curr_mp4_sec;
        struct dentry *de;
        char xattr_value[XATTR_MAX_SIZE];
        char path[PATH_LEN];
        int rc;

        de = d_find_alias(inode);
        if (!de) {
                //pr_err("dentry is null\n");
                rc = 0;
                // rc -EACCES;
                goto out;
        }
        dentry_path_raw(de, path, PATH_LEN - 1);
        /* skip over certain paths heavily used during boot time */
        if (mp4_should_skip_path(path)) {
                rc = 0;
                goto out;
        }
        if (!inode->i_op->getxattr) {
                /*not support*/
                rc = 0;
                goto out;
        }
        curr_mp4_sec = current_security();
        rc = inode->i_op->getxattr(de, XATTR_NAME_MP4, xattr_value, XATTR_MAX_SIZE);
        if (rc <= 0) {
                /*no attribute, grant*/
                rc = 0;
        } else {
                rc = mp4_has_permission(inode, curr_mp4_sec->mp4_flags, 
                                __cred_ctx_to_sid(xattr_value), mask);
        }
out:
        if (de) {
                dput(de);
        }
        return rc;
}


/*
 * This is the list of hooks that we will using for our security module.
 */
static struct security_hook_list mp4_hooks[] = {
        /*
         * inode function to assign a label and to check permission
         */
        LSM_HOOK_INIT(inode_init_security, mp4_inode_init_security),
        LSM_HOOK_INIT(inode_permission, mp4_inode_permission),

        /*
         * setting the credentials subjective security label when laucnhing a
         * binary
         */
        LSM_HOOK_INIT(bprm_set_creds, mp4_bprm_set_creds),

        /* credentials handling and preparation */
        LSM_HOOK_INIT(cred_alloc_blank, mp4_cred_alloc_blank),
        LSM_HOOK_INIT(cred_free, mp4_cred_free),
        LSM_HOOK_INIT(cred_prepare, mp4_cred_prepare)
};

static __init int mp4_init(void)
{
        /*
         * check if mp4 lsm is enabled with boot parameters
         */
        if (!security_module_enable("mp4"))
                return 0;

        pr_info("mp4 LSM initializing..");

        /*
         * Register the mp4 hooks with lsm
         */
        security_add_hooks(mp4_hooks, ARRAY_SIZE(mp4_hooks));

        return 0;
}

/*
 * early registration with the kernel
 */
security_initcall(mp4_init);
