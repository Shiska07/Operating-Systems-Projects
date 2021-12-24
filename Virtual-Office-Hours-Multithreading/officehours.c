#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

/*** Constants that define parameters of the simulation ***/

#define MAX_SEATS 3        /* Number of seats in the professor's office */
#define professor_LIMIT 10 /* Number of students the professor can help before he needs a break */
#define MAX_STUDENTS 1000  /* Maximum number of students in the simulation */

#define CLASSA 0
#define CLASSB 1

/* TODO */
/* Add your synchronization variables here */
sem_t s_inoffice, prof_busy, entered_or_left, A_not_in_office, B_not_in_office, A_waiting, B_waiting;
pthread_mutex_t prof_in_break, mtx1, ask_question;

/* Basic information about simulation.  They are printed/checked at the end 
 * and in assert statements during execution.
 *
 * You are responsible for maintaining the integrity of these variables in the 
 * code that you develop. 
 */

static int students_in_office;   /* Total numbers of students currently in the office */
static int classa_inoffice;      /* Total numbers of students from class A currently in the office */
static int classb_inoffice;      /* Total numbers of students from class B in the office */
static int students_since_break = 0;

// These variables will be used to fulfill requirement 2 nd 4
static int entry_blocked;        /* to check is entry of additional students is blocked */
static int conseq_stud_A;
static int conseq_stud_B;

// These variables will be used to control the flow of students
static int first_student;
static int student_0;
static int entered;
static int left;

typedef struct 
{
    int arrival_time;  // time between the arrival of this student and the previous student
    int question_time; // time the student needs to spend with the professor
    int student_id;
    int class;
} student_info;

/* Called at beginning of simulation.  
 * TODO: Create/initialize all synchronization
 * variables and other global variables that you add.
 */
static int initialize(student_info *si, char *filename) 
{
    students_in_office = 0;
    classa_inoffice = 0;
    classb_inoffice = 0;
    students_since_break = 0;
    entry_blocked = 0;
    first_student = 1;
    conseq_stud_A;
    conseq_stud_B;

  /* Initialize your synchronization variables (and 
   * other variables you might use) here
   */


  /* Read in the data file and initialize the student array */
    FILE *fp;

    if((fp=fopen(filename, "r")) == NULL) 
    {
        printf("Cannot open input file %s for reading.\n", filename);
        exit(1);
    }

    int i = 0;
    while ( (fscanf(fp, "%d%d%d\n", &(si[i].class), &(si[i].arrival_time), &(si[i].question_time))!=EOF) && i < MAX_STUDENTS ) 
    {
        i++;
    }

    fclose(fp);
    return i;
}

static void take_break() 
{
    printf("The professor is taking a break now.\n");
    sleep(5);
    assert( students_in_office == 0 );
    students_since_break = 0;
}

void *professorthread(void *junk) 
{
    printf("The professor arrived and is starting his office hours\n");

    if ( student_0 == CLASSA )
    {
        sem_post( & B_not_in_office );
    }
    else if ( student_0 == CLASSB )
    {
        sem_post( & A_not_in_office );
    }

    /* Loop while waiting for students to arrive. */
    while (1) 
    {
    // wait for a student to enter or leave as these are critical regions
        sem_wait( & entered_or_left );
    // guard critical region
        pthread_mutex_lock( & mtx1 );
        int A_count, B_count;
        sem_getvalue( & A_waiting, & A_count );
        sem_getvalue( & B_waiting, & B_count );


        if ( entered )
        {
        // take a break if professor limit reached by locking mutex &prof_in_break
            if ( students_since_break == professor_LIMIT && !entry_blocked )
            {
                pthread_mutex_lock( & prof_in_break );
                entry_blocked = 1;
            }
            entered = 0;
        }

        if ( entry_blocked && students_in_office == 0 )
        {
            take_break();

            // note that students have been unblocked from entering since prof is done taking a break
            entry_blocked = 0;
            // set left = 0 since no students have left since the break
            left = 0;
            // release mutex after break
            pthread_mutex_unlock( & prof_in_break );

            if ( conseq_stud_A == 5 && B_count != 0 )
            {
                // let B enter
                sem_post( & A_not_in_office );
            }
            else if ( conseq_stud_B == 5 && A_count != 0 )
            {
                // let A enter
                sem_post( & B_not_in_office );
            }
            // if both A and B are waiting after the break
            else if ( A_count != 0 && B_count != 0 )
            {
                int n = rand() % 2;
                if ( n == CLASSA )
                {
                    // let A enter
                    sem_post( & B_not_in_office );
                    
                }
                else if ( n == CLASSB )
                {
                    // let B enter
                    sem_post( & A_not_in_office );
                }
                
            }
            // only students from A waiting
            else if ( B_count == 0 && A_count != 0 )
            {
                // let A enter
                sem_post( & B_not_in_office );
            }
            // only students from B waiting
            else if ( A_count == 0 && B_count != 0 )
            {
                // let B enter
                sem_post( & A_not_in_office );
            }

        }

        //   if a student has just left and prof_limit not reached
        if ( !first_student && left && students_since_break != professor_LIMIT )
        {
            //  no students in office
            if ( students_in_office == 0 )
            {
                //printf("all students have left.\n");
                if ( conseq_stud_A == 5 && B_count != 0 )
                {
                    // let B enter
                    sem_post( & A_not_in_office );
                }
                else if ( conseq_stud_B == 5 && A_count != 0 )
                {
                    // let A enter
                    sem_post( & B_not_in_office );
                }
                // if students from A and B are waiting outside
                else if ( A_count != 0 && B_count != 0 )
                {
                    int n = rand() % 2;
                    if ( n == CLASSA )
                    {
                        // let A enter
                        sem_post( & B_not_in_office );
                    
                    }
                    else if ( n == CLASSB )
                    {
                        // let B enter
                        sem_post( & A_not_in_office );
                    }
                
                }
                // only students from A waiting
                else if ( B_count == 0 && A_count != 0 )
                {   
                    // let A enter
                    sem_post( & B_not_in_office );
                }
                // only students from B waiting
                else if ( A_count == 0 && B_count != 0 )
                {
                    // let B enter
                    sem_post( & A_not_in_office );
                }
            }
        
            left = 0;
        }

        if ( first_student )
        {
            first_student = 0;
        }

        pthread_mutex_unlock( & mtx1 );

        // increment &prof_busy as this will now allow students to eanter or leave
        sem_post( & prof_busy );
    }
    pthread_exit(NULL);
}

void classa_enter() 
{
    
    students_in_office += 1;
    students_since_break += 1;
    classa_inoffice += 1;
    conseq_stud_A += 1;
    conseq_stud_B = 0;

    // student A entered and is no longer waiting
    entered = 1;
    sem_wait( & A_waiting );
    int A_count, B_count;
    sem_getvalue( & A_waiting, & A_count );
    sem_getvalue( & B_waiting, & B_count );
    if ( (students_since_break != professor_LIMIT) && (A_count != 0) && !((conseq_stud_A == 5) && (B_count != 0)))
    {
        sem_post( & B_not_in_office );
    }

}

void classb_enter() 
{
  
    students_in_office += 1;
    students_since_break += 1;
    classb_inoffice += 1;
    conseq_stud_B += 1;
    conseq_stud_A = 0;
  
    // student B entered and is no longer waiting
    entered = 1;
    sem_wait( & B_waiting );
    int A_count, B_count;
    sem_getvalue( & A_waiting, & A_count );
    sem_getvalue( & B_waiting, & B_count );
    if ( (students_since_break != professor_LIMIT) && (B_count != 0) && !((conseq_stud_B == 5) && (A_count != 0)))
    {
        sem_post( & A_not_in_office );
    }

}

static void ask_questions(int t) 
{
    sleep(t);
}

static void classa_leave() 
{

    students_in_office -= 1;
    classa_inoffice -= 1;
    // decrease the count of remaining students from class A
    left = 1;

}

static void classb_leave() 
{

    students_in_office -= 1;
    classb_inoffice -= 1;
    // decrease the count of remaining students from class B 
    left = 1;

}

void* classa_student(void *si) 
{

    sem_post( & A_waiting );
    sem_wait( & B_not_in_office );
    sem_wait( & s_inoffice );                      /* cannot enter if office at full capacity */

    /* A atusent can only enter if prof_busy > 0 which will take place
      only after the prof thread runs to eveluate the situation after
      each students enters and leaves the office                       */

    // &mtx1 guards critical regions common to all threads    
    pthread_mutex_lock( & prof_in_break );         /* cannot enter if prof is taking a break */
    sem_wait( & prof_busy );               
    pthread_mutex_lock( & mtx1 );
  
  
    student_info *s_info = (student_info*)si;

    /* enter office */
    classa_enter();

    printf("Student %d from class A enters the office\n", s_info->student_id);

    assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
    assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);
    assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
    assert(classb_inoffice == 0 );
    
    /* the order of these three functions ensures that the professor thread will
        receive the mutex mtx1 after a student has left                           */
    // release mtx1 to allow prof thread access to critical region
    pthread_mutex_unlock( & mtx1 );
    // release &prof_in_break just in case it is time for professor to have a break
    pthread_mutex_unlock( & prof_in_break );
    // increment value of semaphore to remove blockade from prof thread 
    sem_post( & entered_or_left );
    
    pthread_mutex_lock( & ask_question );
    /* ask questions  --- do not make changes to the 3 lines below*/
    printf("Student %d from class A starts asking questions for %d minutes\n", s_info->student_id, s_info->question_time);
    ask_questions(s_info->question_time);
    printf("Student %d from class A finishes asking questions and prepares to leave\n", s_info->student_id);
    pthread_mutex_unlock( & ask_question );

    /* as classx_leave() is a critical region, it waits for the professor thread to 
        increment the semaphore and release the mutex mtx1                            */
    sem_wait( & prof_busy );
    pthread_mutex_lock( & mtx1 );
    /* leave office */
    classa_leave();  

    printf("Student %d from class A leaves the office\n", s_info->student_id);
    
    assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
    assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
    assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);

    pthread_mutex_unlock( & mtx1 );
    // this removes the blockade on prof thread and allows it to run as a student has left
    sem_post( & entered_or_left );
    // this makes a spot available in the office since a student has left
    sem_post( & s_inoffice );

    pthread_exit(NULL);
}

void* classb_student(void *si)
{

    sem_post( & B_waiting );
    sem_wait( & A_not_in_office );
    sem_wait( & s_inoffice );
    pthread_mutex_lock( & prof_in_break );
    sem_wait( & prof_busy );
    pthread_mutex_lock( & mtx1 );            
    
    student_info *s_info = (student_info*)si;

    /* enter office */
    classb_enter();

    printf("Student %d from class B enters the office\n", s_info->student_id);

    assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
    assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
    assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);
    assert(classa_inoffice == 0 );

    pthread_mutex_unlock( & mtx1 );
    pthread_mutex_unlock( & prof_in_break );
    sem_post( & entered_or_left );

    pthread_mutex_lock( & ask_question );
    printf("Student %d from class B starts asking questions for %d minutes\n", s_info->student_id, s_info->question_time);
    ask_questions(s_info->question_time);
    printf("Student %d from class B finishes asking questions and prepares to leave\n", s_info->student_id);
    pthread_mutex_unlock( & ask_question );

    sem_wait( & prof_busy );
    pthread_mutex_lock( & mtx1 );

    /* leave office */
    classb_leave();      

    printf("Student %d from class B leaves the office\n", s_info->student_id);

    assert(students_in_office <= MAX_SEATS && students_in_office >= 0);
    assert(classb_inoffice >= 0 && classb_inoffice <= MAX_SEATS);
    assert(classa_inoffice >= 0 && classa_inoffice <= MAX_SEATS);
    
    pthread_mutex_unlock( & mtx1 );
    sem_post( & entered_or_left );
    sem_post( & s_inoffice );
    

    pthread_exit(NULL);
}

int main(int nargs, char **args) 
{
    int i;
    int result;
    int student_type;
    int num_students;
    void *status;
    pthread_t professor_tid;
    pthread_t student_tid[MAX_STUDENTS];
    student_info s_info[MAX_STUDENTS];

    if (nargs != 2) 
    {
        printf("Usage: officehour <name of inputfile>\n");
        return EINVAL;
    }

    num_students = initialize(s_info, args[1]);
    if (num_students > MAX_STUDENTS || num_students <= 0) 
    {
        printf("Error:  Bad number of student threads. "
            "Maybe there was a problem with your input file?\n");
        return 1;
    }

    printf("Starting officehour simulation with %d students ...\n",num_students);

    sem_init( & s_inoffice, 0, MAX_SEATS );
    sem_init( & prof_busy, 0, 1 );
    sem_init( & entered_or_left, 0, 0 );
    sem_init( & A_not_in_office, 0, 0 );
    sem_init( & B_not_in_office, 0, 0 );
    sem_init( & A_waiting, 0, 0 );
    sem_init( & B_waiting, 0, 0 );
    pthread_mutex_init( & prof_in_break, NULL );
    pthread_mutex_init( & mtx1, NULL );
    pthread_mutex_init( & ask_question, NULL );

    student_0 = s_info[0].class;

    result = pthread_create(&professor_tid, NULL, professorthread, NULL);

    if (result) 
    {
        printf("officehour:  pthread_create failed for professor: %s\n", strerror(result));
        exit(1);
    }

    for (i=0; i < num_students; i++) 
    {

        s_info[i].student_id = i;
        sleep(s_info[i].arrival_time);
                    
        student_type = random() % 2;

        if (s_info[i].class == CLASSA)
        {
            result = pthread_create(&student_tid[i], NULL, classa_student, (void *)&s_info[i]);
        }
        else // student_type == CLASSB
        {
            result = pthread_create(&student_tid[i], NULL, classb_student, (void *)&s_info[i]);
        }

        if (result) 
        {
            printf("officehour: thread_fork failed for student %d: %s\n", 
                i, strerror(result));
            exit(1);
        }
    }

    /* wait for all student threads to finish */
    for (i = 0; i < num_students; i++) 
    {
        pthread_join(student_tid[i], &status);
    }

    sem_destroy( & s_inoffice );
    sem_destroy( & prof_busy );
    sem_destroy( & entered_or_left );
    sem_destroy( & A_not_in_office );
    sem_destroy( & B_not_in_office );
    sem_destroy( & A_waiting );
    sem_destroy( & B_waiting );

    /* tell the professor to finish. */
    pthread_cancel(professor_tid);

    printf("Office hour simulation done.\n");

    return 0;
}
