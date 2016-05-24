/*  ----------------------------------------------------------------<Prolog>-
    Name:       usemailudb.c
    Title:      UltraSource Unicode Email Daemon Database Access

    Generated by eslservice at 2003/03/18 16:58:27

    Synopsis:   Get data from email log and email queue

    This program is copyright (c) 1991-2001 iMatix Corporation.
    ---------------------------------------------------------------</prolog>-*/

#include "sfl.h"
#include <sqlext.h>
#include "usemailudb.h"

/*- Definition --------------------------------------------------------------*/

#define ERR_MSG_SIZE           300
#define ERR_CODE_SIZE          20

/*- Global Variable ---------------------------------------------------------*/

HENV  
    environment = NULL;                /*  ODBC environment                 */
HDBC
    connection  = NULL;                /*  ODBC connection handle           */
HSTMT
    log_stmt    = NULL,                /* ODBC Statement handle             */
    queue_stmt  = NULL;                /* ODBC Statement handle             */
RETCODE
    rc,
    retcode;
int
    feedback,                          /* feedback value                    */
    err_code;                          /* Error code ( 0 = NO ERROR)        */
static char
    err_code_msg [ERR_CODE_SIZE],      /* Error Message code                */
    err_message  [ERR_MSG_SIZE];       /* Error message                     */

    long   h_usid;
    long   h_ussender;
    UCODE   h_usrecipients [255 + 1];
    UCODE   h_usfromaddr [255 + 1];
    UCODE   h_usreplyaddr [255 + 1];
    UCODE   h_ussubject [150 + 1];
    UCODE   h_usbody [4000 + 1];
    long   h_usstatus;
    UCODE   h_usmessage [4000 + 1];
    double h_ussentat;
    long   h_uscodepage;
    UCODE   h_uscharset [50 + 1];
    double h_uscreatedts;
    long   h_usid;
    double h_ussendat;
    long   h_usemaillogid;
    double h_usrevisedts;

static SDWORD h_usrecipients_ind
    ,h_usfromaddr_ind
    ,h_usreplyaddr_ind
    ,h_ussubject_ind
    ,h_usbody_ind
    ,h_usmessage_ind
    ,h_uscharset_ind
    ;

/*- Local function declaration ---------------------------------------------*/

static int check_sql_error  (HSTMT statement);
void       prepare_log_data (USEMAILULOG *record);

/*##########################################################################*/
/*######################### COMMON FUNCTIONS ###############################*/
/*##########################################################################*/

/*  ---------------------------------------------------------------------[<]-
    Function: usemailu_connect

    Synopsis: Connect to a odbc database.
    ---------------------------------------------------------------------[>]-*/

Bool 
usemailu_connect (char *name, char *user, char *pwd)
{
    Bool
        feedback = FALSE;
    ASSERT (name);

    retcode = SQLAllocEnv (&environment); /* Environment handle              */
    if (retcode != SQL_SUCCESS)
      {
        coprintf ("Error: usemailu_connect (SQLAllocEnv) of %s (%ld)",
                  name, retcode);
        return (FALSE);
      }
    retcode = SQLAllocConnect (environment, &connection);
    if (retcode == SQL_SUCCESS
    ||  retcode == SQL_SUCCESS_WITH_INFO)
      {
        /* Set login timeout to 30 seconds.                                  */
        SQLSetConnectOption (connection, SQL_LOGIN_TIMEOUT, 30);
        /* Connect to data source                                            */
        retcode = SQLConnect(connection, name, SQL_NTS, user, SQL_NTS,
                             pwd, SQL_NTS);
        if (retcode != SQL_SUCCESS
        &&  retcode != SQL_SUCCESS_WITH_INFO)
          {
            check_sql_error (NULL);
            SQLFreeConnect (connection);
            SQLFreeEnv     (environment);
          }
        else
            feedback = TRUE;
      }
    else
      {
        check_sql_error (NULL);
        SQLFreeEnv (environment);
      }
    return (feedback);
}

/*  ---------------------------------------------------------------------[<]-
    Function: usemailu_disconnect

    Synopsis: 
    ---------------------------------------------------------------------[>]-*/

void
usemailu_disconnect (void)
{
    SQLDisconnect  (connection);
    SQLFreeConnect (connection);
    SQLFreeEnv     (environment);
}

/*  ---------------------------------------------------------------------[<]-
    Function: usemailu_error_message

    Synopsis: 
    ---------------------------------------------------------------------[>]-*/

char *
usemailu_error_message (void)
{
    return (err_message);
}

/*##########################################################################*/
/*######################### USEMAILLOG FUNCTIONS #############################*/
/*##########################################################################*/

/*  ---------------------------------------------------------------------[<]-
    Function: usemailulog_update

    Synopsis: update a record into the table USEMAILLOG
    ---------------------------------------------------------------------[>]-*/
int
usemailulog_update (USEMAILULOG *record)
{
    static char update_buffer [] =
        "UPDATE USEMAILLOG SET "                                              \
            "  USSENDER           = ?"                                        \
            ", USRECIPIENTS       = ?"                                        \
            ", USFROMADDR         = ?"                                        \
            ", USREPLYADDR        = ?"                                        \
            ", USSUBJECT          = ?"                                        \
            ", USSTATUS           = ?"                                        \
            ", USSENTAT           = ?"                                        \
            ", USCODEPAGE         = ?"                                        \
            ", USCHARSET          = ?"                                        \
            ", USCREATEDTS        = ?"                                        \
        " WHERE ("                                                            \
            "USID                 = ?)";

    retcode = SQLAllocStmt (connection, &log_stmt);
    if (retcode != SQL_SUCCESS
    &&  retcode != SQL_SUCCESS_WITH_INFO)
      {
        check_sql_error (log_stmt);
        return (feedback);         
      }
    prepare_log_data (record);

    rc = SQLPrepare (log_stmt, update_buffer, SQL_NTS);
    if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
      {
        rc  = SQLBindParameter (log_stmt, 1,                             SQL_PARAM_INPUT,
                                SQL_C_LONG,  SQL_INTEGER, 0, 0,
                                &h_ussender, 0,NULL);
        rc  = SQLBindParameter (log_stmt, 2,                             SQL_PARAM_INPUT,
                                SQL_C_WCHAR,  SQL_WVARCHAR, 255, 0,
                                h_usrecipients,  0, NULL);
        rc  = SQLBindParameter (log_stmt, 3,                             SQL_PARAM_INPUT,
                                SQL_C_WCHAR,  SQL_WVARCHAR, 255, 0,
                                h_usfromaddr,   0, NULL);
        rc  = SQLBindParameter (log_stmt, 4,                             SQL_PARAM_INPUT,
                                SQL_C_WCHAR,  SQL_WVARCHAR, 255, 0,
                                h_usreplyaddr,  0, NULL);
        rc  = SQLBindParameter (log_stmt, 5,                             SQL_PARAM_INPUT,
                                SQL_C_WCHAR,  SQL_WVARCHAR, 150, 0,
                                h_ussubject,    0, NULL);
        rc  = SQLBindParameter (log_stmt, 6,                             SQL_PARAM_INPUT,
                                SQL_C_LONG,  SQL_INTEGER, 0, 0,
                                &h_usstatus, 0,NULL);
        rc  = SQLBindParameter (log_stmt, 7,                             SQL_PARAM_INPUT,
                                SQL_C_DOUBLE,  SQL_DOUBLE, 0, 0,
                                &h_ussentat,     0, NULL);
        rc  = SQLBindParameter (log_stmt, 8,                             SQL_PARAM_INPUT,
                                SQL_C_LONG,  SQL_INTEGER, 0, 0,
                                &h_uscodepage, 0,NULL);
        rc  = SQLBindParameter (log_stmt, 9,                             SQL_PARAM_INPUT,
                                SQL_C_WCHAR,  SQL_WVARCHAR, 50, 0,
                                h_uscharset,    0, NULL);
        rc  = SQLBindParameter (log_stmt, 10,                            SQL_PARAM_INPUT,
                                SQL_C_DOUBLE,  SQL_DOUBLE, 0, 0,
                                &h_uscreatedts,  0, NULL);
        rc  = SQLBindParameter (log_stmt, 11,                            SQL_PARAM_INPUT,
                                SQL_C_LONG,  SQL_INTEGER, 0, 0,
                                &h_usid, 0,NULL);

        if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
            rc = SQLExecute (log_stmt);
      }
    check_sql_error (log_stmt);

    SQLFreeStmt (log_stmt, SQL_DROP);
    return (feedback);
}

/*  ---------------------------------------------------------------------[<]-
    Function: usemailulog_get

    Synopsis: get a record from the table USEMAILLOG
    ---------------------------------------------------------------------[>]-*/

int
usemailulog_get (USEMAILULOG *record)
{
    static char select_statement [] =
        "SELECT "                                                             \
         "  USID"                                                             \
         ", USSENDER"                                                         \
         ", USRECIPIENTS"                                                     \
         ", USFROMADDR"                                                       \
         ", USREPLYADDR"                                                      \
         ", USSUBJECT"                                                        \
         ", USBODY"                                                           \
         ", USSTATUS"                                                         \
         ", USMESSAGE"                                                        \
         ", USSENTAT"                                                         \
         ", USCODEPAGE"                                                       \
         ", USCHARSET"                                                        \
         ", USCREATEDTS"                                                      \
         " FROM USEMAILLOG WHERE"                                             \
         "(USID = ? )";
    SDWORD
        indicator;

    prepare_log_data (record);

    retcode = SQLAllocStmt (connection, &log_stmt);
    if (retcode != SQL_SUCCESS
    &&  retcode != SQL_SUCCESS_WITH_INFO)
      {
        check_sql_error (log_stmt);
        return (feedback);         
      }
    rc = SQLPrepare (log_stmt, select_statement, SQL_NTS);
    if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
      {
        rc  = SQLBindParameter (log_stmt, 1,                             SQL_PARAM_INPUT,
                                SQL_C_LONG,  SQL_INTEGER, 0, 0,
                                &h_usid, 0,NULL);
        if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
            rc = SQLExecute (log_stmt);
      }
    check_sql_error (log_stmt);
    if (feedback == OK)
      {
        rc = SQLBindCol (log_stmt, 1, 
                         SQL_C_LONG,   &h_usid, 0, &indicator);
        rc = SQLBindCol (log_stmt, 2, 
                         SQL_C_LONG,   &h_ussender, 0, &indicator);
        rc = SQLBindCol (log_stmt, 3, 
                         SQL_C_WCHAR,  &h_usrecipients, (255 + 1) * UCODE_SIZE, &h_usrecipients_ind);
        rc = SQLBindCol (log_stmt, 4, 
                         SQL_C_WCHAR,  &h_usfromaddr, (255 + 1) * UCODE_SIZE, &h_usfromaddr_ind);
        rc = SQLBindCol (log_stmt, 5, 
                         SQL_C_WCHAR,  &h_usreplyaddr, (255 + 1) * UCODE_SIZE, &h_usreplyaddr_ind);
        rc = SQLBindCol (log_stmt, 6, 
                         SQL_C_WCHAR,  &h_ussubject, (150 + 1) * UCODE_SIZE, &h_ussubject_ind);
        rc = SQLBindCol (log_stmt, 7, 
                         SQL_C_WCHAR,  &h_usbody, (4000 + 1) * UCODE_SIZE, &h_usbody_ind);
        rc = SQLBindCol (log_stmt, 8, 
                         SQL_C_LONG,   &h_usstatus, 0, &indicator);
        rc = SQLBindCol (log_stmt, 9, 
                         SQL_C_WCHAR,  &h_usmessage, (4000 + 1) * UCODE_SIZE, &h_usmessage_ind);
        rc = SQLBindCol (log_stmt, 10, 
                         SQL_C_DOUBLE, &h_ussentat, 0, &indicator);
        rc = SQLBindCol (log_stmt, 11, 
                         SQL_C_LONG,   &h_uscodepage, 0, &indicator);
        rc = SQLBindCol (log_stmt, 12, 
                         SQL_C_WCHAR,  &h_uscharset, (50 + 1) * UCODE_SIZE, &h_uscharset_ind);
        rc = SQLBindCol (log_stmt, 13, 
                         SQL_C_DOUBLE, &h_uscreatedts, 0, &indicator);
        rc = SQLFetch (log_stmt);
        if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
          {
            check_sql_error (log_stmt);
            feedback = RECORD_NOT_PRESENT;
          }
        else
          {
            memset (record, 0, sizeof (USEMAILULOG));
            record-> usid                 = h_usid;
            record-> ussender             = h_ussender;
            record-> usstatus             = h_usstatus;
            record-> uscodepage           = h_uscodepage;
            if (h_usreplyaddr_ind < 0)
                *h_usreplyaddr                       = (UCODE)'\0';
            else
                h_usreplyaddr [h_usreplyaddr_ind / UCODE_SIZE] = (UCODE)'\0';
            wcscpy (record-> usreplyaddr,         h_usreplyaddr);
            if (h_ussubject_ind < 0)
                *h_ussubject                         = (UCODE)'\0';
            else
                h_ussubject [h_ussubject_ind / UCODE_SIZE] = (UCODE)'\0';
            wcscpy (record-> ussubject,           h_ussubject);
            if (h_usbody_ind < 0)
                *h_usbody                            = (UCODE)'\0';
            else
                h_usbody [h_usbody_ind / UCODE_SIZE] = (UCODE)'\0';
            wcscpy (record-> usbody,              h_usbody);
            if (h_usrecipients_ind < 0)
                *h_usrecipients                      = (UCODE)'\0';
            else
                h_usrecipients [h_usrecipients_ind / UCODE_SIZE] = (UCODE)'\0';
            wcscpy (record-> usrecipients,        h_usrecipients);
            if (h_usmessage_ind < 0)
                *h_usmessage                         = (UCODE)'\0';
            else
                h_usmessage [h_usmessage_ind / UCODE_SIZE] = (UCODE)'\0';
            wcscpy (record-> usmessage,           h_usmessage);
            if (h_uscharset_ind < 0)
                *h_uscharset                         = (UCODE)'\0';
            else
                h_uscharset [h_uscharset_ind / UCODE_SIZE] = (UCODE)'\0';
            wcscpy (record-> uscharset,           h_uscharset);
            if (h_usfromaddr_ind < 0)
                *h_usfromaddr                        = (UCODE)'\0';
            else
                h_usfromaddr [h_usfromaddr_ind / UCODE_SIZE] = (UCODE)'\0';
            wcscpy (record-> usfromaddr,          h_usfromaddr);
            record-> ussentat             = h_ussentat;
            record-> uscreatedts          = h_uscreatedts;
          }
      }
    SQLFreeStmt (log_stmt, SQL_DROP);
    return (feedback);
}

/*##########################################################################*/
/*######################## EMAILQUEUE FUNCTIONS ############################*/
/*##########################################################################*/

/*  ---------------------------------------------------------------------[<]-
    Function: usemailuqueue_get

    Synopsis: Get all email queue record in alocated record table
    Return the number of record in table.
    ---------------------------------------------------------------------[>]-*/

long
usemailuqueue_get_next (double timestamp, USEMAILUQUEUE *record)
{
    static char select_statement [] =
        "SELECT TOP 1 USID, USSENDAT, USEMAILLOGID, USREVISEDTS FROM USEMAILQUEUE " \
        "WHERE (USSENDAT <= ? AND USEMAILLOGID > 0) ORDER BY USSENDAT ASC";
    static char count_statement [] =                                          \
         "SELECT count(*) FROM USEMAILQUEUE WHERE (USSENDAT <= ? AND USEMAILLOGID > 0)";
    SDWORD
        indicator;
    long
        rec_count = 0;

    memset (record, 0, sizeof (USEMAILUQUEUE));
    h_ussendat = timestamp; 
    retcode = SQLAllocStmt (connection, &queue_stmt);
    if (retcode != SQL_SUCCESS
    &&  retcode != SQL_SUCCESS_WITH_INFO)
      {
        check_sql_error (queue_stmt);
        return (feedback);         
      }

    /* Get count of record                                                   */
    rc = SQLPrepare (queue_stmt, count_statement, SQL_NTS);
    if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
      {
        rc  = SQLBindParameter (queue_stmt, 1, SQL_PARAM_INPUT,
                                SQL_C_DOUBLE,  SQL_DOUBLE, 0, 0,
                                &h_ussendat,     0, NULL);
        if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
            rc = SQLExecute (queue_stmt);
        check_sql_error (queue_stmt);
      }
    if (feedback == OK)
      {
        rc = SQLBindCol (queue_stmt, 1, 
                         SQL_C_LONG,   &rec_count, 0, &indicator);
        rc = SQLFetch (queue_stmt);
        if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
          {
            check_sql_error (queue_stmt);
            feedback = RECORD_NOT_PRESENT;
          }
      }

    if (rec_count > 0)
      {
        SQLFreeStmt (queue_stmt, SQL_CLOSE);
        rc = SQLPrepare (queue_stmt, select_statement, SQL_NTS);
        if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
         {
            rc  = SQLBindParameter (queue_stmt, 1, SQL_PARAM_INPUT,
                            SQL_C_DOUBLE,  SQL_DOUBLE, 0, 0,
                             &h_ussendat,     0, NULL);
            if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
                 rc = SQLExecute (queue_stmt);
            check_sql_error (queue_stmt);
         }
        if (feedback == OK)
          {
            rc = SQLBindCol (queue_stmt, 1, 
                     SQL_C_LONG,   &h_usid, 0, &indicator); 
            rc = SQLBindCol (queue_stmt, 2, 
                     SQL_C_DOUBLE, &h_ussendat, 0, &indicator);
            rc = SQLBindCol (queue_stmt, 3, 
                     SQL_C_LONG,   &h_usemaillogid, 0, &indicator);
            rc = SQLBindCol (queue_stmt, 4, 
                     SQL_C_DOUBLE, &h_usrevisedts, 0, &indicator);
            rc = SQLFetch (queue_stmt);
            if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
              {
                check_sql_error (queue_stmt);
                feedback = RECORD_NOT_PRESENT;
              }
            else
              {
                record-> usid         = h_usid;
                record-> usemaillogid = h_usemaillogid;
                record-> ussendat     = h_ussendat;
                record-> usrevisedts  = h_usrevisedts;
              }
         }
      }
    SQLFreeStmt (queue_stmt, SQL_DROP);
    return (rec_count);
}


/*  ---------------------------------------------------------------------[<]-
    Function: usemailuqueue_delete

    Synopsis: delete a record into the table USEMAILQUEUE
    ---------------------------------------------------------------------[>]-*/

int
usemailuqueue_delete     (USEMAILUQUEUE *record)
{
    HSTMT
        queue_stmt;
    static char delete_record [] =
        "DELETE FROM USEMAILQUEUE WHERE ( "                                  \
            "USID = ?)";
    h_usid                = record-> usid; 


    retcode = SQLAllocStmt (connection, &queue_stmt);
    if (retcode != SQL_SUCCESS
     &&  retcode != SQL_SUCCESS_WITH_INFO)
      {
        check_sql_error (queue_stmt);
        return (feedback);         
      }

    rc = SQLPrepare (queue_stmt, delete_record, SQL_NTS);
    if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
      {
        rc  = SQLBindParameter (queue_stmt, 1,                           SQL_PARAM_INPUT,
                                SQL_C_LONG,  SQL_INTEGER, 0, 0,
                                &h_usid, 0,NULL);
        if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO)
            rc = SQLExecute (queue_stmt);
      }
    check_sql_error (queue_stmt);
    SQLFreeStmt (queue_stmt, SQL_DROP);
    return (feedback);         
}

/*##########################################################################*/
/*######################### INTERNAL FUNCTIONS #############################*/
/*##########################################################################*/

/*  ------------------------------------------------------------------------
    Function: check_sql_error - INTERNAL

    Synopsis: Check the SQL error
    ------------------------------------------------------------------------*/

static int
check_sql_error (HSTMT statement)
{
    long
        size;
    short
        size2;

    err_code = 0;
    
    if (rc == SQL_SUCCESS
    ||  rc == SQL_SUCCESS_WITH_INFO)
        err_code = 0;
    else
      {
        SQLError (environment, connection, statement, err_code_msg, &size,
                  err_message, ERR_MSG_SIZE - 1, &size2);
        if (strcmp (err_code_msg, "23000") == 0)
            err_code = -1;
        else
        if (strcmp (err_code_msg, "00000") == 0)
            err_code = 1;
        else
            err_code = -2;
      }

    if (err_code > 0)
      {
        feedback = RECORD_NOT_PRESENT;
      }
    else
    if (err_code == -1)
      {
        feedback = DUPLICATE_RECORD;
      }
    else
    if (err_code < 0)
      {
        feedback = HARD_ERROR;
        coprintf ("SQL Error    %d : %s", err_code, err_message);
      }
    else
        feedback = OK;

    return (feedback);
}


/*  ------------------------------------------------------------------------
    Function: prepare_log_data - INTERNAL

    Synopsis:
    ------------------------------------------------------------------------*/

static void
prepare_log_data (USEMAILULOG *record)
{
    h_usid                 = record-> usid; 
    h_ussender             = record-> ussender; 
    h_usstatus             = record-> usstatus; 
    h_uscodepage           = record-> uscodepage; 
    wcscpy (h_usreplyaddr,   record-> usreplyaddr);
    wcscpy (h_ussubject,     record-> ussubject);
    wcscpy (h_usbody,        record-> usbody);
    wcscpy (h_usrecipients,  record-> usrecipients);
    wcscpy (h_usmessage,     record-> usmessage);
    wcscpy (h_uscharset,     record-> uscharset);
    wcscpy (h_usfromaddr,    record-> usfromaddr);
    h_ussentat             = record-> ussentat; 
    h_uscreatedts          = record-> uscreatedts; 
}
