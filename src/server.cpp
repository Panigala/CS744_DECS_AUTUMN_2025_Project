#include <iostream>
#include <unordered_map>
#include <list>
#include <mutex>
#include <sstream>
#include <iomanip>
#include <atomic>
#include "httplib.h"
#include <mysql/mysql.h>


using namespace std;
constexpr size_t CACHE_CAPACITY = 5000; //max capacity of cache




/*=============================================================
                        MySQL connecting
 ===============================================================*/
MYSQL* connect_db() {
    // Initialize a new MySQL connection handle, mysql_init() prepares a connection object for further use.
    MYSQL *conn = mysql_init(nullptr);
    if (!conn) return nullptr; // If initialization failed, return null (connection object invalid).

    /* Try connecting to the MySQL server using provided credentials:
    - Host: 127.0.0.1  (local machine)
    - User: user_744
    - Password: key_744
    - Database: key_value_DB_744
    - Port: 0 (default MySQL port 3306 will be used)
    - Socket and client flags set to nullptr and 0 */
    if (!mysql_real_connect(conn, "127.0.0.1", "user_744", "key_744", "key_value_DB_744", 0, nullptr, 0)) {
        cerr << "MySQL connection failed: " << mysql_error(conn) << endl; // If connection fails, print an error message from MySQL.
        mysql_close(conn);// Close and free the connection handle to avoid leaks.
        return nullptr; // Return nullptr to indicate failure to caller.
    }

    // Create the key-value table if it doesn’t already exist. with - `k`: VARCHAR(255) used as the key (PRIMARY KEY ensures uniqueness) 
    mysql_query(conn, "CREATE TABLE IF NOT EXISTS key_value_table (k VARCHAR(255) PRIMARY KEY, v TEXT)");
    return conn;// Return the valid connection object to the caller, This will be used throughout the program to perform SQL operations.
}

// function to escape single quotes in SQL strings, Prevents SQL injection or syntax errors by escaping `'` as `\'`.
string escape_sql(const string &s) {
    string out;
    for (char c : s)        // Iterate through every character in the input string.
        out += (c == '\'' ? "\\'" : string(1, c));   // If the character is a single quote ('), append an escaped version.
    return out;  // Return the sanitized SQL-safe string.
}






/*=============================================================
                         LRU cache
================================================================*/
class LRUCache {
public:
    explicit LRUCache(size_t capacity) : capacity_(capacity) {} // Constructor — initialize the cache with a maximum capacity.


    // GET from cache
    bool get(const string &key, string &value) {
        lock_guard<mutex> lg(mu_);          // cache mutex
        get_requests_++;                    // Increment GET operation count declared in private
        total_requests_++;                  // Increment total requests count (all types)
        auto it = map_.find(key);                        // Try to find the key in the hashmap
        if (it == map_.end()) {             // Not found → cache miss
            get_misses_++;                  // Increment GET misses
            total_misses_++;                // Increment total misses
            return false;                   // Indicate cache miss
        }
        items_.splice(items_.begin(), items_, it->second); // Found in cache → move the accessed item to front of LRU list (most recently used)
        value = it->second->second;         // Extract the value associated with this key or callers variable
        get_hits_++;                        // Increment GET hit count
        total_hits_++;                      // Increment total hit count
        return true;                        // Indicate success
    }


    // PUT/POST — insert or update in cache
    void put(const string &key, const string &value) {
        lock_guard<mutex> lg(mu_);          // cache mutex
        auto it = map_.find(key);
        if (it != map_.end()) {             // If key already exists → update value and move to front
            it->second->second = value;
            items_.splice(items_.begin(), items_, it->second);
            return;
        }
        // If cache is full → remove LRU (back of list)
        if (items_.size() >= capacity_) {
            const auto &last = items_.back();
            map_.erase(last.first);         // Erase from map
            items_.pop_back();              // Remove from list
        }
        // Insert new key-value pair at the front (MRU)
        items_.emplace_front(key, value);
        map_[key] = items_.begin();         // Point map entry to list node
    }


    // DELETE — remove from cache if exists
    void erase(const string &key) {
        lock_guard<mutex> lg(mu_);          // cache mutex
        auto it = map_.find(key);
        if (it != map_.end()) {             // if found in map then
            items_.erase(it->second);       // Erase from linked list
            map_.erase(it);                 // Erase from map
        }
    }


    // POPULAR stats counter
    void count_popular_access() {
        lock_guard<mutex> lg(mu_);          // cache mutex
        pop_requests_++;                    // Count POPULAR endpoint call
        total_requests_++;                  // Count in total requests too
        if (!items_.empty()) {                      // If cache has any data
            pop_hits_++;                    // Consider as a cache "hit"
            total_hits_++;
        } else {                            // Empty cache
            pop_misses_++;                  // No data = "miss"
            total_misses_++;
        }}
    vector<string> keys() const {                   // Return all keys in MRU order (front = most recently used)
        lock_guard<mutex> lg(mu_);          // cache mutex
        vector<string> ks;
        ks.reserve(items_.size());          // Preallocate memory
        for (const auto &p : items_)        // Traverse linked list 'items_'
            ks.push_back(p.first);          // Collect keys
        return ks;                          // Return all key names
    }


    // cache stats report in JSON 
    string stats_json() const {
        lock_guard<mutex> lg(mu_);

        // to compute the hit ratio
        auto ratio = [](long h, long m){return (h + m == 0) ? 0.0 : (100.0 * h / (double)(h + m));};
       
        stringstream ss; // Build a JSON string manually (no external JSON library)
        ss << fixed << setprecision(2);
        ss << "{\n"
           << "  \"cache_size\": " << items_.size() << ",\n"        // Current number of items
           << "  \"cache_capacity\": " << capacity_ << ",\n"        // Max possible capacity
           << "  \"per_operation\": {\n"
           << "    \"GET\": {\"requests\": " << get_requests_       // GET stats
           << ", \"hits\": " << get_hits_
           << ", \"misses\": " << get_misses_
           << ", \"hit_ratio\": " << ratio(get_hits_, get_misses_) << "},\n"
           << "    \"POPULAR\": {\"requests\": " << pop_requests_   // POPULAR stats
           << ", \"hits\": " << pop_hits_
           << ", \"misses\": " << pop_misses_
           << ", \"hit_ratio\": " << ratio(pop_hits_, pop_misses_) << "}\n"
           << "  },\n"
           << "  \"cumulative\": {\"requests\": " << total_requests_// All ops combined
           << ", \"hits\": " << total_hits_
           << ", \"misses\": " << total_misses_
           << ", \"hit_ratio\": " << ratio(total_hits_, total_misses_) << "}\n"
           << "}";
        return ss.str();                                            // Return full JSON string
    }


private:
    size_t capacity_;                        // Max cache size (number of key-value pairs)
    mutable mutex mu_;                       // Mutex for thread-safe access

    // LRU data structures:
    list<pair<string, string>> items_;       // Doubly-linked list storing {key, value} pairs, Front = Most Recently Used (MRU), Back = Least Recently Used (LRU)
    unordered_map<string, list<pair<string, string>>::iterator> map_;// Fast O(1) lookup from key → iterator into list
                                             
    // Stats counters — atomic to avoid race conditions for hit,miss etc.. counter updating
    atomic<long> get_hits_{0}, get_misses_{0}, get_requests_{0};   // GET stats
    atomic<long> pop_hits_{0}, pop_misses_{0}, pop_requests_{0};   // POPULAR stats
    atomic<long> total_hits_{0}, total_misses_{0}, total_requests_{0}; // Total across all ops
    
    //!!!!!!!!!!!!!!!!!!add here if want another stats , according to access ++,--  and return jason also modify
};




/*=============================================================
                 main server logic
================================================================*/
int main() {
    LRUCache cache(CACHE_CAPACITY);//creating instance of LRU cache with specified capacity
    MYSQL *conn = connect_db(); //establish a connection to the MySQL database using the connect_db()function.
    if (!conn) { cerr << "DB connection failed\n"; return 1; } 

    mutex db_mutex;  //mutex for accessing database
    httplib::Server server;  //instantiate the HTTP server object from the httplib library.




    // ---------- PUT and POST end points handler ---------- 
    // This block defines a shared handler that both PUT and POST endpoints will use because they both semantically same.
    auto handle_put_post = [&](const httplib::Request &request, httplib::Response &response) {
        string key = request.matches[1]; // Extract the key part from the URL path (captured by the regex `/(.+)`). eg :  PUT /kv/user1 → key = "user1"
        string val = request.body; // Extract the value from the request body.
            
        // writing to DB getting mutex for it
        lock_guard<mutex> lock(db_mutex); 
        // Construct an SQL query that inserts or replaces the key-value pair.
        string q = "REPLACE INTO key_value_table (k,v) VALUES('" + escape_sql(key) + "','" + escape_sql(val) + "')";
        mysql_query(conn, q.c_str()); // Execute the SQL query on the connected MySQL server.
    
        //after writing to DB update cache ie, write through
        cache.put(key, val);

        response.set_content("OK\n", "text/plain"); // Respond to client confirming successful write. ie, Send HTTP 200 OK response
    };

    // The PUT endpoint calls handle_put_post() for any path matching /kv/<key>. used to insert or update a key-value pair.
    server.Put(R"(/table_key_value/(.+))", handle_put_post);
    // POST behaves identically to PUT here (both perform write-through updates). Some clients may prefer POST for semantic reasons (e.g., creating new data).
    server.Post(R"(/table_key_value/(.+))", handle_put_post);




   // ---------- GET endpoint handles HTTP GET requests for key lookups----------
   // it first checks the cache; if not found, it queries the MySQL database and updates the cache before returning the result.
   server.Get(R"(/table_key_value/(.+))", [&](const httplib::Request &request, httplib::Response &response) {
    string key = request.matches[1], val; // extract the key , val from url request
    
    if (cache.get(key, val)) { //check cache first
        response.set_content(val, "text/plain");
        return;}// if found no need to go to DB just repond

        //cache miss so then aquire DB mutex
        lock_guard<mutex> lock(db_mutex);
        string q = "SELECT v FROM key_value_table WHERE k='" + escape_sql(key) + "' LIMIT 1"; //sql query prepare
        mysql_query(conn, q.c_str()); //execute sql query
        MYSQL_RES *r = mysql_store_result(conn); // Retrieve the query result from MySQL.
        
        if (r) { // Check if any row was returned.
            MYSQL_ROW row = mysql_fetch_row(r);  // Fetch the first row.           
            if (row && row[0]) {           // If the row exists and contains a value:
                val = row[0];              // Extract value from the result.
                cache.put(key, val);       // Store it in cache for future GETs.
                response.set_content(val, "text/plain");  // Send to client.
                mysql_free_result(r);      // Free MySQL result memory.
                return;                    // Exit after responding successfully.
            }
        mysql_free_result(r);} // If no matching key was found in DB, free result memory anyway.
    
        //if bothe cache and DB miss
    response.status = 404;
    response.set_content("Key not found\n", "text/plain");
    });




   //------------DELETE endpoint handles HTTP DELETE requests----------
   server.Delete(R"(/table_key_value/(.+))", [&](const httplib::Request &request, httplib::Response &response) {
        string key = request.matches[1];
        bool found = false;

        // first trying to delete from DB
        {   lock_guard<mutex> lock(db_mutex);
            string q = "DELETE FROM key_value_table WHERE k='" + escape_sql(key) + "'";
            if (mysql_query(conn, q.c_str())) {
                response.status = 500;
                response.set_content(string("DB error: ") + mysql_error(conn), "text/plain");
                return;}
            
                // Check if a row was actually deleted
            if (mysql_affected_rows(conn) > 0)
                found = true;}

        // delete from cache (if it exists)
        cache.erase(key);

        if (found) {
            response.set_content("Deleted\n", "text/plain");
        } else {
            response.status = 404;
            response.set_content("Key not found — cannot delete\n", "text/plain");
        }
    });

    





//---------------popular. Handles GET /popular MRU keys in cache----------------
server.Get("/popular", [&](const httplib::Request &, httplib::Response &response) {
    cache.count_popular_access(); // Increments total/popular request counters, and counts hit/miss
    auto keys = cache.keys(); // Retrieve the current cache keys in Most-Recently-Used (MRU) order.
    size_t n = min<size_t>(10, keys.size()); // Limit the output to at most 10 keys even if cache has more if i put any number instead of 10 then can get upto that
    stringstream ss; // Build a JSON response containing the cached keys and their count.
    ss << "{\n  \"cached_keys\": [";

    // Loop through the top N keys and store to ss as JSON strings.
    for (size_t i = 0; i < n; ++i) {
        ss << "\"" << keys[i] << "\"";   
        if (i + 1 < n) ss << ", ";  }     

    // Add count of returned keys and total number of items in cache.
    ss << "],\n  \"count\": " << n << ", \"total_cached\": " << keys.size() << "\n}";
    response.set_content(ss.str(), "application/json"); // Send the JSON string as HTTP response with proper content type.
});




//---------------stats. Handles GET /stats — returns the current cache performance statistics---------------
server.Get("/stats", [&](const httplib::Request &, httplib::Response &response) {
    string stats_json = cache.stats_json(); // The function `cache.stats_json()` builds this JSON report, its inside the cache.
    response.set_content(stats_json, "application/json"); // Send the JSON statistics as the HTTP response body.content type is set to "application/json" so clients know it's structured data
});




// ---------- health, HTTP GET endpoint at path "/health"  to verify if the server is running and responsive. ----------
server.Get("/health", [&](const httplib::Request &, httplib::Response &response) {
    // Set the HTTP response body content to a simple text message "OK\n".
    response.set_content("OK\n", "text/plain");
});




 // ---------- DEFAULT 404 HANDLER. ie, It will be called automatically whenever any request fails----------
server.set_error_handler([](const httplib::Request &request, httplib::Response &response) {
    if (response.status == 404) { // Check if the error status is 404 → resource not found
        stringstream ss;  // Create a stringstream to build the response text dynamically        
        ss << "404 Not Found\n" // Construct a descriptive error message including the HTTP method and path
           << "The requested resource (" << request.method << " " << request.path << ") does not exist.\n";
        response.set_content(ss.str(), "text/plain"); // Set this as the HTTP response content with MIME type "text/plain"
    }
    else if (response.status == 405) { // If the status is 405 → method not allowed (e.g., POST to a GET-only endpoint)
        response.set_content("405 Method Not Allowed\n", "text/plain");  // Respond with a plain-text message for clarity
    }    
    else if (response.status >= 500) { // If the status is 500 or higher → internal server error (unexpected failures)
        response.set_content("500 Internal Server Error\n", "text/plain"); // Respond with a simple internal server error message
    }
});




    cout << "Server running at http://127.0.0.1:8080\n";
    server.listen("0.0.0.0", 8080);                        //is the one that starts an infinite event loop inside the httplib library. like while(1) so it in kind of blockin state

    //for debugging this not reached cause always listening its kinda blocked 
    mysql_close(conn);
    return 0;
}
