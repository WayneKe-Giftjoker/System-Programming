# Programming HW4 - Simple Movies Recommendation System

> ![:date:](https://assets.hackmd.io/build/emojify.js/dist/images/basic/date.png) Deadline: Fri 16 Dec 23:59:59 CST 2022

> [Github Classroom Link](https://classroom.github.com/a/SQUpGaDC)

> [Video Link](https://youtu.be/ZpgwOeGoqBA)

> [Discussion Space Link](https://github.com/NTU-SP/SP_HW4_release/discussions)

## 1. Problem Description

In this homework, we’re going to implement a simple movies recommendation system. There are two main tasks should be handled.

1. Each request contains a keyword and an user profile, the system should (1) filter the movies by keywords and, (2) do sorting via recommendation scores.
2. Handle multiple search requests simultaneously.

The dataset we used is [MovieLens](https://grouplens.org/datasets/movielens/), you can go to the page if you want to get more information about it.

### Task 1: Filtering and Sorting

#### Content-Based Recommendation

Each request contains a keyword, we first need to filter the movies containing the keyword. Then we should calculate recommendation scores for the filtered movies.

In MovieLens, movies are labeled with corresponding genres and users give movies ratings from 0.5 to 5 stars. We can use an one-hot vector (v) to represent each movie, where 1 means the movie belongs to this genre and 0 means not belongs to it.

User profile (U) can be calculated by sum of (ratings * movie_profile). Then we can calculate recommendation score by cosine simalarity. (normalize U and v, then do dot product)
![img](https://i.imgur.com/9nuPYUR.png)

#### Parallel Merge Sort

After calculating recommendation scores, we are going to sort the movies by the scores. When doing sort, you are strictly to use our sorting library with time complexity of O(n^2) with a network delay (0.1s) provided by TAs. To verify it, TAs will randomly add secret keys to the movie titles to check whether you used the sorting library.

You may need to divide the list of the movies into several parts, delegate each part to a thread, use the sorting library to sort each part, then merge them to get the final result.

The figure below shows the details of the sorting (with depth = 3). You should use the provided sorting library in the lowest level and pick a suitable depth for a better perfomance.
![img](https://i.imgur.com/RYEFMrq.png)

#### Sorting rule:

1. Movies with higher score should be put in the front.
2. If the scores are the same, sort with alphabetical order(strcmp) and put the smaller one in the front.

```
void* sort(char** movies, double* pts, int size);

Example:
sort(["A","B","C"], [30.0,40.0,30.0], 3); 

Result:
movies: ["B","A","C"]
pts: [40.0, 30.0, 30.0]
```

#### Hint:

When implementing with multiprocess, [this page](https://stackoverflow.com/questions/5656530/how-to-use-shared-memory-with-linux-in-c) may help. Also, you can check about the [manpage](https://man7.org/linux/man-pages/man2/mmap.2.html).

### Task 2: Handle Multiple Requests Simultaneously

In some test cases, they will include more than one request. You may need to use multithread to handle multiple requests simultaneously. It might not be possible to expect requests to be processed sequentially within the time limit.

You should be careful to deal with possible race problems when processing requests, since the requests are globally shared. You can use any method taught in this course to protect the shared resources, i.e., critical section, but we recommend you to use mutex since using signal may be more complicated.

## 2. Implementation

### File Structure

```
SP_HW4_release
├── data
│   └── movies.txt
├── header.h
├── lib.c
├── Makefile
├── server.c
└── testcases
    ├── input0.txt
    ├── input1.txt
    ├── input2.txt
    ├── input3.txt
    ├── input4.txt
    └── input5.txt
```

#### 2.1 Makefile

This file will be provided by TA. Feel free to modify it, but the executable file name should not be modified. It should produce `tserver` for using multithread to do merge sort, and `pserver` for using multiprocess to do merge sort.

TA will test your code by running the following commands:

```
make
./pserver < /path/to/input/data
./tserver < /path/to/input/data
```

#### 2.2 server.c

You should implement the recommender system in server.c. TAs have provided initialize code in this file. Feel free to modify the code if needed. Please make sure all the movies you filtered are put into `sort()` in `lib.c`. You can write your code like the following example to split codes for pserver and tserver:

```
#ifdef PROCESS
    fprintf(stderr,"usage: ./pserver");
#elif defined THREAD
    fprintf(stderr,"usage: ./tserver");
#endif
```

#### 2.3 lib.c

This file implement sorting and is provided by TA. You don’t need to submit this file. To check whether your server use this library, TA will add the secret key when judging. If you don’t use this library, your output will not include the secret key and will be seen as wrong answer.

#### 2.4 header.h

This file will be provided by TA. Feel free to modify it.

#### 2.5 movies.txt

This file includes titles and profiles of movies. It will be provided by TA. It should be put under `./data/`.

#### 2.6 input{0…5}.txt

The test cases are provided. They are placed under `./testcases/`

### Input Format

- format:

  ```
  [number of requests]\n
  [request id] [keyword] [user profile]\n
  ... 
  ```

  - The `[request id]` for each request is unique for each testcase.
  - Numbers in the user profile is separated by comma(‘,’). There are 19 genres.
  - Number of requests will be 1~32. Request id is an integer between 0~170000. Maximum length of movie titles and keywords is defined in `header.h`.
  - Regular expression for keywords is `([a-zA-Z0-9]+)|\*`. Movies filtered by keyword are case sensitive.

  Note that if the keyword is `*`, it means that the system should return all of the movies.

- example:

  ```
  2      //num of requests
  0 * 2.5,5.0,5.0,9.5,0.0,3.5,9.5,9.5,4.0,2.5,1.5,6.5,8.0,5.0,0.0,1.0,3.5,5.0,0.0\n
  3 1995 9.5,9.5,4.0,2.5,5.0,5.0,9.5,0.0,1.0,3.5,5.0,0.0,0.0,3.5,2.5,1.5,6.5,8.0,5.0\n
  ```

  There are total two requests in this case. The request with id=0 above will sort all movies in the dataset while the request with id=3 above will sort the movies whose title contains “1995”.

### Output Format

For pserver, you need to write the sorted search result to `{id}p.out` file. For example, for the input with id=0, the filename of the output should be `0p.out`.

For tserver, you need to write the sorted search result to `{id}t.out` file. For example, for the input with id=0, the filename of the output should be `0t.out`.

You only need to write the titles of the movies into output files.

Example:

```
{sp2022}:Mudhoney (1965)\n
{sp2022}:Hard Boiled Mahoney (1947)\n
```

`{sp2022}:` is added by the sorting library. Note that this is just an example and the output is not guaranteed to be the same as above.

## 3. Grading

![:warning:](https://assets.hackmd.io/build/emojify.js/dist/images/basic/warning.png) **Warning:**

- Please strictly follow the implementation guidelines, or TAs’ programs may not work successfully with yours and you will lose points.
- If your submission includes unnecessary files, your grade will be capped at 6.
- Please make sure your submission compiles and runs on CSIE workstations successfully.
- Please strictly follow the output format, or you may not get the full credits.
- Please use `ps -u [student id]` or `htop -u [student id]` to check for dangling processes. To clean up all your processes on a workstation, please execute `pkill -9 -u [student id]`.

- (2pts) Multithread for merge sort (1 request), finish in X seconds and the answers should be correct.
  - (1pts) simple test case, X = 1.0
  - (1pts) hard test case, X = 1.5
- (3pts) Multithread for merge sort (multiple requests), finish in X seconds and the answers should be correct.
  - (1pts) 4 requests (simple test case), X = 1.0
  - (1pts) 8 requests, X = 2.5
  - (1pts) 32 requests, X = 5.0
- (2pts) Multiprocessing with shared memory for merge sort (1 request), the answers should be correct
  - (1pts) simple test case
  - (1pts) hard test case
- (1pts) Report
  - We will provide an input file `input0.txt`, try to specify depth from 1 to 5 in parallel merge sort statically and compare execution time of using threads and processes on `linux{}.csie.ntu.edu.tw` or on your host machine, and plot in one figure. The figure should include results of depth = 1~5 for merge sort. Briefly explain the difference of execution time. Since workstation load is unstable, if you couldn’t get the expected result or not implement parallel merge sort, briefly explain what you expect to get.

**Note:** In order not to let the test result of the running time of the program be affected by the current workload, we will run your program ***k*** times and take the test result with the shortest running time for each testcase. However, please think deeply in your code that will not have race problems. We will check if the program completed the task ***k*** times and the consistency of your results. If the results are inconsistent, or once the program is crashed, then you will not get points of the testcase.

## 4. Submission

### 4.1 To Github

Your source code should be submitted to GitHub before deadline. The submission should include at least three files:

- Makefile
- header.h
- server.c

You are allowed to add other files, however, your submission should not include the files below:

- executable files

*** Update: You don’t need to delete the following files.

- testcases/
- data/
- lib.c

The TAs will clone your repository and test your latest commit on the main branch. We will test your code on `linux1.csie.ntu.edu.tw`. Please make sure your codes work as expected on the server.

Github Classorm seems to have a problem where we can’t record the submission time of each commit. Therefore, we have to clone your github repo manually after the deadlines. **You should not do any operation on your github repo at 17 Dec 2022 00:00 ~ 01:00.** Otherwise, we cannot guarantee the correctness of the cloned repo.

### 4.2 To NTU COOL

Your report should be submitted to NTU COOL before deadline.

## 5. Reminders

- Plagiarism is STRICTLY prohibited.
  - Late policy (D refers to the formal deadline, 12/16 23:59)
  - If you submit your assignment on D+1 or D+2, your score will be multiplied by 0.85.
  - If you submit your assignment between D+3 and D+5, your score will be multiplied by 0.7.
  - If you submit your assignment between D+6 and 12/30, your score will be multiplied by 0.5.
  - Late submission after 12/30 23:59 will not be accepted.

## 6. Where to Ask Questions

1. Evoke QAs in GitHub discussions: 

   NTU-SP/SP_HW4_release

   - [How to open a GitHub Discussion](https://youtu.be/rgCMmg9lBNw?t=210)

2. Send emails to 

   ```
   ntusp2022@gmail.com
   ```

   .

   - Before sending mails, please make sure that you have **read the SPEC carefully**, and **no similar questions were asked** in the discussion space, or TAs will be very tired and upset. ![:upside_down_face:](https://assets.hackmd.io/build/emojify.js/dist/images/basic/upside_down_face.png)

   - **How to send an Email properly**

     - Mail title should be like: `[SP Programming HW4] TA hour reservation`
     - It’s better to use your `@ntu.edu.tw`, `@g.ntu.edu.tw`, `@csie.ntu.edu.tw` email accounts to send the mail.
     - You should write down your student ID, and name at the end of the mail.
     - Please mind your manners.
     - Use punctuation marks to make your email easier to read.
     - If you want to ask questions about code in the mail, please tell us how to reproduce your problem. Attach a screenshot, a video recording, or your code file will. It will be easier for us to understand your situation.
     - Below is an example format:

     ```
     Title: [SP Programming HW4] TA hour reservation
     Sent By: r11952025@csie.ntu.edu.tw
     Contents:
         Dear SP TAs,
         
         Can I have a TA hour appointment at 13:00 - 14:00 Tuesday?
         I encounter some problems with shared memory. Thank you!
     
         Best,
         R11952025 資工碩一 王大明
     ```

   - If you don’t follow the upper rules, we may ignore your email. (Most of you guys are good, though.![:slightly_smiling_face:](https://assets.hackmd.io/build/emojify.js/dist/images/basic/slightly_smiling_face.png))

3. Ask during TA hours

   - Below are the TA hours for this Programming Assignment. Please make appointments by emailing 

     ```
     ntusp2022@gmail.com
     ```

      before visiting.

     - Tuesday 13:00 - 14:00 at Room 404, CSIE building
     - Wednesday 17:30 - 18:30 at Room 302, CSIE building
     - Friday 15:00 - 16:00 at Room 404, CSIE building



# SP_HW4_release

**[SPEC Link](https://hackmd.io/@claire2222/SkLyBSnIo)**

**[Video Link](https://www.youtube.com/watch?v=ZpgwOeGoqBA)**

**[Discussion Space Link](https://github.com/NTU-SP/SP_HW4_release/discussions)**

## How to exec?
In order not to let the test result of the running time of the program be affected by the current workload, we will run your program 3-5 times and take the test result with the shortest running time for each testcase. Meanwhile, we will check the consistency of your results among the tests. If the results are inconsistent, or once the program is crashed, then you will not get points of the testcase. 

Your `Makefile` should produce executable file `tserver` for using multithread to do merge sort  and `pserver` for using multiprocess to do merge sort. The output results `{}.out` of your program should place in your current working dir. TA will pull your repository and replace the `lib.c` library and run with following command to test your program in **linux1.csie.ntu.edu.tw**:  
```
make 
./pserver < /path/to/input/data  
./tserver < /path/to/input/data
```

Remember `make clean` to remove all {}.out and executable file.  
Note that it does not mean you can get full points by successfully running with these two programs in the time limit. We will check about if you have completed the requirements, for instance, using provided sort library, multiprocess programming, i.e., `fork()`.

### Tips
You can quickly test the execution time of your program using following command (note that the variable `real time` in bash using `time` command is counted as time uses):
```
for i in {1..5};do time ./tserver < /path/to/input$i.txt;echo "";done
```
