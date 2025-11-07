#include <iostream>
#include <string>
#include "httplib.h"

using namespace std;

int main() {
    string host = "127.0.0.1";
    int port = 8080;

    httplib::Client tester(host, port); // Create HTTP client
    tester.set_read_timeout(5, 0);      // 5 sec timeout for reponse first param = seconds , second param = 0 in microseconds
    tester.set_write_timeout(5, 0);     // 5 sec timeout for sending or posting first param = seconds , second param = 0 in microseconds

    cout << "================= Test: Key-Value API Server Request and Response =================\n";

    string base_path = "/table_key_value/";
    string key = "testkey";
    string key2 = "testkey2";
    string value = "hello_world";
    string new_value = "updated_value";

    // -------------------- PUT --------------------
    cout << "\n[PUT] Request: " << base_path + key << endl;
    auto put_response = tester.Put((base_path + key).c_str(), value, "text/plain");
    if (put_response)
        cout << "Response (" << put_response->status << "): " << put_response->body << endl;
    else
        cout << "PUT failed (server not responding)\n";


    // -------------------- PUT another key ---------
    cout << "\n[PUT] Request: " << base_path + key2 << endl;
    auto put_response2 = tester.Put((base_path + key2).c_str(), value, "text/plain");
    if (put_response2)
        cout << "Response (" << put_response->status << "): " << put_response->body << endl;
    else
        cout << "PUT failed (server not responding)\n";


    // -------------------- GET --------------------
    cout << "\n[GET] Request: " << base_path + key << endl;
    auto get_response = tester.Get((base_path + key).c_str());
    if (get_response)
        cout << "Response (" << get_response->status << "): " << get_response->body << endl;
    else
        cout << "GET failed\n";

    // -------------------- POST --------------------
    cout << "\n[POST] Request: " << base_path + key << endl;
    auto post_response = tester.Post((base_path + key).c_str(), new_value, "text/plain");
    if (post_response)
        cout << "Response (" << post_response->status << "): " << post_response->body << endl;
    else
        cout << "POST failed\n";

    // -------------------- GET (after POST) --------------------
    cout << "\n[GET after POST] Request: " << base_path + key << endl;
    auto get2_response = tester.Get((base_path + key).c_str());
    if (get2_response)
        cout << "Response (" << get2_response->status << "): " << get2_response->body << endl;
    else
        cout << "GET (after POST) failed\n";

    // // -------------------- DELETE --------------------
    // cout << "\n[DELETE] Request: " << base_path + key << endl;
    // auto delete_response = tester.Delete((base_path + key).c_str());
    // if (delete_response)
    //     cout << "Response (" << delete_response->status << "): " << delete_response->body << endl;
    // else
    //     cout << "DELETE failed\n";


    // -------------------- DELETE again (nonexistent key) --------------------
    cout << "\n[DELETE again] (nonexistent key) Request: " << base_path + key << endl;
    auto delete_response2 = tester.Delete((base_path + key).c_str());
    if (delete_response2)
        cout << "Response (" << delete_response2->status << "): " << delete_response2->body << endl;
    else
        cout << "DELETE (again) failed\n";

    // -------------------- /popular --------------------
    cout << "\n[GET] /popular" << endl;
    auto popular_response = tester.Get("/popular");
    if (popular_response)
        cout << "Response (" << popular_response->status << "): " << popular_response->body << endl;
    else
        cout << "GET /popular failed\n";

    // -------------------- /health --------------------
    cout << "\n[GET] /health" << endl;
    auto health_response = tester.Get("/health");
    if (health_response)
        cout << "Response (" << health_response->status << "): " << health_response->body << endl;
    else
        cout << "GET /health failed\n";

    // -------------------- Invalid endpoint --------------------
    cout << "\n[GET] /invalid_endpoint" << endl;
    auto invalid_response = tester.Get("/invalid_endpoint");
    if (invalid_response)
        cout << "Response (" << invalid_response->status << "): " << invalid_response->body << endl;
    else
        cout << "GET /invalid_endpoint failed\n";

    // -------------------- /stats --------------------
    cout << "\n[GET] /stats" << endl;
    auto stats_response = tester.Get("/stats");
    if (stats_response)
        cout << "Response (" << stats_response->status << "): " << stats_response->body << endl;
    else
        cout << "GET /stats failed\n";

    cout << "\n============================== Test Completed ===========================\n";
    return 0;
}
