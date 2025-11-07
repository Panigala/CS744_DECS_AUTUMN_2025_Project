/*-----------------------------------------------------------------------------
Multi-threaded Load Generator for the HTTP Key-Value Server
-----------------------------------------------------------------------------
Description:
 - Simulates concurrent clients sending GET, PUT, DELETE, and POPULAR requests to your server with user-defined ratios.
 - Generates random text values for PUT operations.
 - Measures throughput, latency, and success rate.
 - Fetches server cache stats (/stats) after test completion.
 - Saves detailed logs for each thread and summary in timestamped folders.(optional)
Build:
  g++ -std=c++17 client.cpp -o client -lpthread

Usage:
  ./client <threads> <duration_seconds> <GET_ratio> <PUT_ratio> <DELETE_ratio> <POPULAR_ratio>

Example:
  ./client 10 20 50 20 20 10
  → 10 threads, run for 20 seconds, ratios: 50% GET, 20% PUT, 20% DELETE, 10% POPULAR
*/

#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <random>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include "httplib.h"

using namespace std;
using namespace std::chrono;

//global atomic counters
atomic<int> total_requests(0);
atomic<long long> total_time_us(0);
atomic<int> total_get(0), total_put(0), total_del(0), total_popular(0);
atomic<int> total_success(0), total_fail(0);

// for generating random string for the value of any key
string random_text(mt19937 &gen, int length = 10) {
    static const string charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    uniform_int_distribution<int> dist(0, charset.size() - 1);
    string s;
    for (int i = 0; i < length; i++)
        s += charset[dist(gen)];
    return s;
}

// thread worker function
void client_worker(int id, int duration, int get_ratio, int put_ratio, int del_ratio, int popular_ratio,
                   const string &logdir) {
    httplib::Client cli("127.0.0.1", 8080);
    cli.set_read_timeout(5, 0); //maximum time wait for response here first argument 5 ie seconds, second argument 0 ie microseconds
    cli.set_write_timeout(5, 0); //maximum time wait sending anything to server here first argument 5 ie seconds, second argument 0 ie microseconds

    // Random generators
    mt19937 gen(random_device{}());
    uniform_int_distribution<int> keydist(1, 10000); // random key range ie, key can be 1 to n, 
    uniform_int_distribution<int> opdist(1, 100);   // choose operation randomly ie total 100 ratio 

    // Per-thread counters
    int get_count = 0, put_count = 0, del_count = 0, popular_count = 0;
    int success_count = 0, fail_count = 0;

    auto start_time = steady_clock::now();// storing the time when thread start

    while (duration_cast<seconds>(steady_clock::now() - start_time).count() < duration) { // comparing current time and start time to check whether it elasped the given duratio  or not?
        int op = opdist(gen);  // random number generaatio between 1 to 100 for selecting which operation to do now
        int key = keydist(gen); //raandom key generaation
        string path = "/table_key_value/" + to_string(key); //generating request url endpoint
        string value = random_text(gen, 12); // generate random string for value
        bool ok = false;  // for determining the request success or failure
    
        try {
            httplib::Result result;// to store the http response

            //selecting operation based on random number and ratio and starting clock for measuring time for each request and adding to global time
            if (op <= get_ratio) {//GET
                auto t_start = high_resolution_clock::now();
                result = cli.Get(path.c_str());
                auto t_end = high_resolution_clock::now();
                total_time_us += duration_cast<microseconds>(t_end - t_start).count();
                get_count++;

            } else if (op <= get_ratio + put_ratio) {//PUT/POST
                auto t_start = high_resolution_clock::now();
                result = cli.Put(path.c_str(), value, "text/plain");
                auto t_end = high_resolution_clock::now();
                total_time_us += duration_cast<microseconds>(t_end - t_start).count();
                put_count++;

            } else if (op <= get_ratio + put_ratio + del_ratio) {//DELETE
                auto t_start = high_resolution_clock::now();
                result = cli.Delete(path.c_str());
                auto t_end = high_resolution_clock::now();
                total_time_us += duration_cast<microseconds>(t_end - t_start).count();
                del_count++;

            } else {//popular
                auto t_start = high_resolution_clock::now();
                result = cli.Get("/popular");
                auto t_end = high_resolution_clock::now();
                total_time_us += duration_cast<microseconds>(t_end - t_start).count();
                popular_count++;
            }

            // Any handled response >= 200 and not equals to 404 counts as success
            if (result && result->status == 200 )
                ok = true;

        } catch (...) {// if no reponse from server etc...
            ok = false;
        }
        
        //update succes or fail req counters
        total_requests++;
        if (ok)
            success_count++;
        else
            fail_count++;
    }

    // Merge per-thread stats into global totals
    total_get += get_count;
    total_put += put_count;
    total_del += del_count;
    total_popular += popular_count;
    total_success += success_count;
    total_fail += fail_count;

    /* -------------------------------------------------------------
     for debugging per thread details Save per-thread log
    / -------------------------------------------------------------*/
    // string logname = logdir + "/thread_" + to_string(id) + ".log";
    // ofstream log(logname);
    // log << "Thread " << id << " Summary\n";
    // log << "------------------------\n";
    // log << "GET ops: " << get_count << "\n";
    // log << "PUT ops: " << put_count << "\n";
    // log << "DELETE ops: " << del_count << "\n";
    // log << "POPULAR ops: " << popular_count << "\n";
    // log << "Success: " << success_count << "\n";
    // log << "Fail: " << fail_count << "\n";
    // log.close();
}




//Main function
int main(int argc, char *argv[]) {
    if (argc < 7) {
        cerr << "Usage: ./client <threads> <duration_seconds> <GET_ratio> <PUT_ratio> <DELETE_ratio> <POPULAR_ratio>\n";
        cerr << "Example: ./client 10 20 50 20 20 10\n";
        return 1;
    }
    //converting command line arguments to integer from string
    int threads = stoi(argv[1]);
    int duration = stoi(argv[2]);
    int get_ratio = stoi(argv[3]);
    int put_ratio = stoi(argv[4]);
    int del_ratio = stoi(argv[5]);
    int popular_ratio = stoi(argv[6]);

    // ensure ratios sum to 100
    if (get_ratio + put_ratio + del_ratio + popular_ratio != 100) {
        cerr << "Error: Ratios must sum to 100 (GET + PUT + DELETE + POPULAR = 100)\n";
        return 1;
    }

    // Create timestamped directory for logs
    auto now = chrono::system_clock::now();
    time_t t = chrono::system_clock::to_time_t(now);
    tm local_tm = *localtime(&t);
    stringstream ts;
    ts << put_time(&local_tm, "%Y-%m-%d_%H-%M-%S");
    string logdir = "loader_thread_log/" + to_string(threads)+"_threads"+ "/" + ts.str() + "_time=" + to_string(duration) + "_GET=" + to_string(get_ratio)+ "_PUT=" + to_string(put_ratio)+ "_DEL=" + to_string(del_ratio)+ "_POP=" + to_string(popular_ratio); //directory name etc.. loader_thread_log/<number of threads>/<datetime>,........
    filesystem::create_directories(logdir); //creating the directory

    //just print for diagnostics what currently running i mean what kind of load
    cout << "==========================================================\n";
    cout << "Starting Load Test\n";
    cout << "Threads: " << threads << ", Duration: " << duration << " sec\n";
    cout << "Ratios → GET: " << get_ratio << "%, PUT: " << put_ratio
         << "%, DELETE: " << del_ratio << "%, POPULAR: " << popular_ratio << "%\n";
    cout << "Logs directory: " << logdir << "\n";
    cout << "==========================================================\n";

    // Spawn threads
    vector<thread> workers;
    for (int i = 0; i < threads; i++)
        workers.emplace_back(client_worker, i, duration, get_ratio, put_ratio, del_ratio, popular_ratio, logdir);
    for (auto &t : workers) t.join();

    // compute metrics avg lat, through put request/sec etc...
    double avg_latency_ms = (double)total_time_us / total_requests / 1000.0;
    double throughput_rps = (double)total_requests / duration;
    double success_rate = 100.0 * total_success / total_requests;

    // fetch server stats ie, cache hit rate etc..
    httplib::Client cli("127.0.0.1", 8080);
    auto stats = cli.Get("/stats");
    string cache_info = (stats && stats->status == 200) ? stats->body : "Cache stats unavailable";

    // write summary log
    string summary_path = logdir + "/summary.log";
    ofstream summary(summary_path);
    summary << "========= Load Test Summary =========\n";
    summary << "Threads: " << threads << "\n";
    summary << "Duration: " << duration << " sec\n";
    summary << "Total Requests: " << total_requests << "\n";
    summary << "GET: " << total_get << " | PUT: " << total_put << " | DELETE: " << total_del << " | POPULAR: " << total_popular << "\n";
    summary << "Successful responses: " << total_success << " (" << success_rate << "%)\n";
    summary << "Failed responses: " << total_fail << "\n";
    summary << "Throughput: " << throughput_rps << " req/s\n";
    summary << "Avg latency: " << avg_latency_ms << " ms\n";
    summary << "====================================\n\n";
    summary << "Server Cache Stats:\n" << cache_info << "\n"; //cache stats
    summary.close();

    // print summary on terminal 
    cout << fixed << setprecision(2);
    cout << "\n========= Load Test Summary =========\n";
    cout << "Total threads: " << threads << "\n";
    cout << "Total requests: " << total_requests << "\n";
    cout << "GET: " << total_get << " | PUT: " << total_put << " | DELETE: " << total_del << " | POPULAR: " << total_popular << "\n";
    cout << "Successful responses: " << total_success << " (" << success_rate << "%)\n";
    cout << "Failed responses: " << total_fail << "\n";
    cout << "Throughput: " << throughput_rps << " req/s\n";
    cout << "Avg latency: " << avg_latency_ms << " ms\n";
    cout << "====================================\n";
    cout << "Logs saved in: " << logdir << "\n";
    // cout << "Server Cache Stats:\n" << cache_info << "\n"; //cache stats

    return 0;
}
