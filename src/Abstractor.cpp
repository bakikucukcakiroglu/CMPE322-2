/*
* 
*@file Abstractor.cpp
*@author Hasan Baki Küçükçakıroğlu
*@name and student ID Hasan Baki Küçükçakıroğlu, 2018400141
*
* 
*/

// INCLUDED LIBRARIES //
#include <iostream>
#include <string>
#include <string.h>
#include <set>
#include <sstream>  
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <queue>
#include <utility>
#include <vector>
#include <fstream>
#include <iomanip>
#include <fcntl.h>
#include <pthread.h>
#include <algorithm>
#include <deque>
#include <thread>
#include <mutex>

using namespace std;

struct Result { // The result of the threads will go to main thread thanks to the help of the this struct.
	string abstract_name, summary; // Abstract name and its summary.
	double score; // Score of the result.
};

struct Thread_Input { // The input to the threads will go main memory to threads thanks to help of the this struct.
	string name; // Name of the thread.
	set<string> query_set; // Query words sent as an input to the thread.
};


bool compare_by_field(struct Result &a, struct Result & b){ // After threads end, We will compare the results using this function and sort() function.
	return a.score>b.score; // Return true if first parameter is larger than second parameter.
}

ifstream in_file; // Input file. ( Global declared )
ofstream out_file; // Output file ( Global declared )
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER; // Mutex. ( Global initialized. )

queue<pair<string,string> > abstracts; // First of pair is abstract name and the second of pair is abstract content. To not lose the order and in order to access by threads, use global queue. 
vector<Result> results; // Global vector for holding the results of the scannings of abstracts which is done by the threads.

void* similarity_calculator(void * param){ // The function which threads will operate on. This function will calculate similarity score and will find also the summary and then returns them to the global declared data structures.

	while(!abstracts.empty()){ // Until global abstracts queue is not empty do...

		set<string> abstract_set; // For non duplicate abstract content.
		set<string> query_set; // For non duplicate query words.

		pthread_mutex_lock(&mutex1); // Lock the critical section, we will manipulate shared data structures !

		struct Thread_Input* struct_param = (struct Thread_Input*)param; // For taking thread inputs.

		query_set=struct_param->query_set; // Take the query words to the query set.

		if(abstracts.empty()){		// We will check whether abstract queue is empty or not because if it is we do not want threads to try pop from an empty queue.
			pthread_mutex_unlock(&mutex1); 	// Unlock the lock, we are done with shared data structures.
			break; // If queue is empty break from the loop and end the thread !
		}

		string abstract_name=abstracts.front().first;	// Record the name of the abstract which is scanned by current thread now.

		string abstract_text= abstracts.front().second; // Record the content of the abstract which is scanned by the current thread now.
	
		abstracts.pop(); // Pop the recorded abstract.

		out_file<<"Thread "<<struct_param->name<<" is calculating "<<abstract_name<<endl; // Write output file that which thread scans which abstract.

		pthread_mutex_unlock(&mutex1); 	// Unlock the lock, we are done with shared data structures.


	   set<string>::iterator it; // For iterating through set.

	   vector<int>intersect_sentence_numbers; // For holding intersected sentence numbers.

	   istringstream abstract_divider(abstract_text); // For diving abstract content to the its words.

       int sentence_cnt=0; // The sentence count which is scanned.

	   int intersect=0; // The intersected word index

	   set<string> intersected_words; // Set for holding intersected words.
		
	   string word; // String for holding words of the abstract content.

		while(abstract_divider>>word){  // Fill all the query words to the global query words vector
	
			abstract_set.insert(word); // Insert the word to the abstract_set. We will eliminate duplicate words.

			set<string> ::iterator itr; // Iterator for iterating set.

			for (itr = query_set.begin(); itr != query_set.end(); itr++) { // Iterate through query set. Go query word by query word.

				if( word==*itr){ // If query word is equal to abstract sentence word.

					if(intersect_sentence_numbers.size()==0||(intersect_sentence_numbers[intersect_sentence_numbers.size()-1]!=sentence_cnt)){
						intersect_sentence_numbers.push_back(sentence_cnt); // Record the sentence which there is a query word.
					}
					intersected_words.insert(word); // Record the words that is intersected.
				}
			}	
			if(word=="."){ // If sentence is completed.
				sentence_cnt++; // Increase the sentence count.
			}
		}

		vector<string> sentences; // Vector for recording sentences.
		istringstream abstract_divider_sentence(abstract_text); // Split the abstract content token by token.
		string token; // String for holding token

		while(std::getline(abstract_divider_sentence, token, '.')) { // If token is equal to dot.
			token=token+".";
			sentences.push_back(token); // Push the sentence back.
		}

		string summary=""; // String for forming summary.
		for(auto index : intersect_sentence_numbers){ // For intersecting sentences
			summary+=sentences[index]; // Record that sentences to the summary string.
		}

		vector<string> intersecting; // Vector for intersecting words 
		vector<string> the_union;  // Vector for union of abstract content words and the query.

		set_intersection(abstract_set.begin(), abstract_set.end(), query_set.begin(), query_set.end(), std::back_inserter(intersecting)); // Find the intersecting words and record them to the intersecting vector.
		set_union(abstract_set.begin(), abstract_set.end(), query_set.begin(), query_set.end(), std::back_inserter(the_union));		  // Find the union of abstract content and the query words and record them into the the union vector.
		

		pthread_mutex_lock(&mutex1); // Lock the critical section, we will manipulate shared data structures !

		double score= (double)intersecting.size() / (double)the_union.size(); // Record the result of similarity result formula to the double with doing type casting.
		
		struct Result res; // Struct for recording output of the thread and which we will use that in order to send the results to the main thread.
		res.abstract_name=abstract_name; // Record the abstract name which was scanned.
		res.summary=summary; // Record the last summary of the abstract.
		res.score=score; // Record the similarity formula result.
		results.push_back(res); // Push back this struct to the global vector.

		pthread_mutex_unlock(&mutex1); 	// Unlock the lock, we are done with shared data structures.
	}
	pthread_exit(0); // End the thread.
}

int main(int argc, char** argv){

	string input_file= argv[1]; // Take the name of the input file as an argument from the console.
	string output_file=argv[2]; // Take the name of the output file as an argument from the console.

	vector<char>processor_names; // Vector for thread names. It is initialized later in the below in the order of A...Z .

	for(int i=0; i<26; i++){

		processor_names.push_back ( 65 + i); // Initialize the thread names vector from A to Z.
	}

	in_file.open(input_file); // Open the input file.
	out_file.open(output_file); // Open the output file.
	string first_line; // String for the line by line parsing.
	getline(in_file, first_line); // Take the first line.

	istringstream first_line_divider(first_line); // Divide the first line.

	int T; //Number of threads (other than the main thread) that will be created for the session
	int A; //Number of abstracts that will be scanned by the threads
	int N; //Number of abstracts that will be returned and summarized

	first_line_divider>>T>>A>>N; // Take T,A,N as inputs from the first line of the input file.

	string query_string; // String for holding query words.
	getline(in_file, query_string); // Take the second line as the query string.

	istringstream query_divider(query_string); // Divide the query to the words.

	set<string> query_set; // Query set for holding non duplicate query words.

	int k=0;
	string word;

	while(query_divider>>word){  //Fill all the query words to the global query words vector
		k++;
		query_set.insert(word); // Insert the words to the query set.
	}
	string abstract_name; // String for holding abstract names.

	while(getline(in_file, abstract_name)){  // Until there are abstract names left continue to execute.

		string abstract_content; // String for holding abstract content.

		ifstream abstract_file; // For opening given abstract.

		abstract_file.open("../abstracts/"+abstract_name); // This opens the abstract file which is located in the previous location.

		pair<string, string> abstract; // Pair data structure for holding the name of the abstract and its content. First field is holding the name of the abstract and the second holds the content of the abstract.
		abstract.first= abstract_name; // Hold the name.
		abstract.second=""; // Will hold the content.

		string abstract_line;
		while(getline(abstract_file, abstract_line)){
			abstract.second+= abstract_line; // Now second field holding the content line by line.
		}
		abstracts.push(abstract); // Push the completed pair to the global abstract pair holder queue.
	}

	pthread_t ids[T];  // Thread ids as number of threads.

	struct Thread_Input params[T]; // Thread Input struct as the number of threads.

	for(int i=0; i<T; i++){ // For the number of threads do...
		params[i].name=processor_names[i]; // Assign the name of the thread.
		params[i].query_set= query_set; // Assign the query words to the thread.
		pthread_create(&ids[i], NULL, similarity_calculator, &params[i]);	// Create the thread and give the inputs and the function which thread will operate on.
	}

	int i=0;
	while(i<T){ // For all threads do...
		pthread_join(ids[i], NULL); // Wait for threads to finish.
		i++;
	}

	sort(results.begin(), results.end(), compare_by_field); // Sort the results with given comparing function.
	i=0;

	while(i<N){ // For number of results wanted do....
		// Write the results to the output file.

		if(results[i].summary[0]!=' '){
			
			out_file<<"###"<<endl<< "Result " << i + 1 << ":" <<endl<<"File: "<< results[i].abstract_name << endl<< "Score: " << fixed << setprecision(4) << results[i].score << endl<< "Summary: " << results[i].summary<< endl;	


		}else{

			out_file<<"###"<<endl<< "Result " << i + 1 << ":" <<endl<<"File: "<< results[i].abstract_name << endl<< "Score: " << fixed << setprecision(4) << results[i].score << endl<< "Summary:" << results[i].summary<< endl;	

		}
		i++;
	}
	out_file << "###" << endl;						
	// End of the code.

	return 0;


}