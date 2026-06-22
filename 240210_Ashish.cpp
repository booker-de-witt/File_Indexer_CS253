#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <stdexcept>
#include <memory>
using namespace std;


bool isAlphaNum(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

string toLower(std::string str) {
    string result = "";
    for (size_t i = 0; i < str.length(); ++i) {
        char c = str[i];
        if (c >= 'A' && c <= 'Z') {
            result += (c - 'A' + 'a');
        } else {
            result += c;
        }
    }
    return result;
}


    
class BufferFileReader {
	private:
		string filename;
        int buffer_size;
		ifstream file;
		string leftover;
		char *buffer;

public:
	BufferFileReader(const string &file_name, int buffer_size_kb) : filename(file_name), buffer_size(buffer_size_kb * 1024), leftover("") {

		if (buffer_size_kb < 256 || buffer_size_kb > 1024) {
			throw std::invalid_argument("Buffer size must be between 256 KB and 1024 KB.");
		}

		file.open(filename, ios::binary);

		if(!file.is_open()){
			throw runtime_error("Failed to open file: " + filename);
		}

		buffer = new char[buffer_size];
	}

	~BufferFileReader(){
		if(file.is_open()) file.close();
		delete[] buffer;
	}

	bool readChunk(string& out_chunk){
		if(!file && leftover.empty()) return false;

		file.read(buffer,buffer_size);
		streamsize bytes_read = file.gcount();

		if(bytes_read == 0 && leftover.length()==0) return false;

		string curr_chunk = leftover;
		curr_chunk.append(buffer,bytes_read);

		// Handling the case when a word gets cut in half due to fixed buffer size

		leftover = "";
		int split_pos = -1;


		for(int i= curr_chunk.length()-1; i >=0; --i){
			if(!isAlphaNum(curr_chunk[i])){
				split_pos = i;
				break;
			}
		}

		if(split_pos != -1){
			leftover = curr_chunk.substr(split_pos+1);
			curr_chunk = curr_chunk.substr(0,split_pos+1);
		}

		out_chunk = curr_chunk;
		return bytes_read>0 || out_chunk.length()>0;

	}

    // Function Overloading
	bool readChunk(string& out , int something_here){
        return false;
    }
};


class Tokenize{
    public:
        virtual ~Tokenize(){}
        virtual vector<string> tokenize(string text) = 0;
};


class Tokenizer :public Tokenize{
    public:
        vector<string> tokenize(string text) override {
            vector<string> tokens;
            long long int w_start=0,w_end;

            for(int i = 0; i < text.length();i++){
                if(!isAlphaNum(text[i])){
                    w_end = i;
                    if(w_start < w_end){
                        tokens.push_back(text.substr(w_start,w_end-w_start));
                    }
                    w_start = i+1;
                }
            }

            if (w_start < text.length()) {
    tokens.push_back(text.substr(w_start));
}


            return tokens;
            
        }
};




class VersionedIndexer {
    private: 
        unordered_map<string,long long> word_freq;
        string version_name;

    public:
        VersionedIndexer(string & v_name) : version_name(v_name){

        }

        void processFile(const string &filename, int buffer_size_kb){
            BufferFileReader reader(filename, buffer_size_kb);
            Tokenizer tokenizer;
            string chunk;

            while(reader.readChunk(chunk)){
                vector<string> tokens = tokenizer.tokenize(chunk);
                for(const string & token: tokens){
                    word_freq[toLower(token)]++;
                }
            }
        }


        long long getWordCount(const string &word) const {
            string lower_word = toLower(word);
            auto it = word_freq.find(lower_word);
            return it != word_freq.end() ? it->second:0;
        }


        vector<pair<string,long long>> getTopK(int k) const{
            vector<pair<string,long long >> freq_vec(word_freq.begin(),word_freq.end());

            auto comparator = [](const pair<string,long long>a , const pair<string, long long> b){
                if (a.second != b.second) return a.second > b.second;
                return a.first < b.first;
            };

            sort(freq_vec.begin(),freq_vec.end(),comparator);
            if( freq_vec.size() > k){
                freq_vec.resize(k);
            }

            return freq_vec;
        }

        string getVersionName() const {return version_name;}
};




class QueryProcessor {
    public:
        virtual ~QueryProcessor() = default;
        virtual void execute() = 0;
};


class WordQuery : public QueryProcessor {
    private:
        VersionedIndexer &indexer;
        string word;
    public:
        WordQuery(VersionedIndexer& idx, string &st) : indexer(idx), word(st){}

        void execute() override {
            long long count = indexer.getWordCount(word);
            cout << "Version: " << indexer.getVersionName() << endl;
            cout << "Query result: " << word << " -> " << count << endl;
        }

};

class TopKQuery : public QueryProcessor {
    private:
        VersionedIndexer& indexer;
        int k;

    public:
        TopKQuery(VersionedIndexer& idx, int k_) : indexer(idx), k(k_){}

        void execute() override {
            auto top_words = indexer.getTopK(k);
            cout << "Version: " << indexer.getVersionName() << endl;
            cout << "Query result (Top " << k << "): " << endl;
            for(const auto& pair: top_words){
                cout << pair.first  << " -> " << pair.second << endl; 
            }
        }
};


class DiffQuery: public QueryProcessor {
    private:
        VersionedIndexer& indexer1;
        VersionedIndexer& indexer2;
        string word;

    public:
        DiffQuery(VersionedIndexer& idx1, VersionedIndexer &idx2 , string& w): indexer1(idx1), indexer2(idx2) , word(w){}

        void execute() override {
            long long count1 = indexer1.getWordCount(word);
            long long count2 = indexer2.getWordCount(word);
            // long long diff = count1- count2;

            std::cout << "Versions: " << indexer1.getVersionName() << ", " << indexer2.getVersionName() << std::endl;
            std::cout << "Query result (Diff for '" << word << "'): " << count2-count1 << std::endl;       
        }
};



int main(int argc, char* argv[]){
    try{
        auto start_time = std::chrono::high_resolution_clock::now();

        unordered_map<string, string> args;
        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];
            if (arg.rfind("--", 0) == 0 && i + 1 < argc) {
                args[arg.substr(2)] = argv[++i];
            }
        }


        if(args.find("buffer") == args.end()){
            throw invalid_argument("Buffer size is required");
        }
        long long buffer_size_kb = stoll(args["buffer"]);

        if(args.find("query") == args.end()){
            throw invalid_argument("Query Type is required");
        }
        string query_type = args["query"];

        QueryProcessor* processor = nullptr;
        VersionedIndexer* idx1 = nullptr;
        VersionedIndexer* idx2 = nullptr;

        if(query_type == "word" || query_type == "top"){
            if (args.find("file") == args.end() || args.find("version") == args.end()) {
                throw invalid_argument("File and version are required for single-version queries.");
            }

            idx1 = new VersionedIndexer(args["version"]);
            idx1->processFile(args["file"],buffer_size_kb);

            if(query_type == "word"){
                if(args.find("word") == args.end()) invalid_argument("Word is required for the word query");

                processor = new WordQuery(*idx1,args["word"]);
            } else {
                if(args.find("top") == args.end()) invalid_argument("Top K is required for the top k query");
                processor = new TopKQuery(*idx1, stoi(args["top"]));

            }
        }
        else if (query_type == "diff"){
            if (args.find("file1") == args.end() || args.find("version1") == args.end() || args.find("file2") == args.end() || args.find("version2") == args.end())
                throw invalid_argument("Both files and versions are required for diff queries.");
    
            if (args.find("word") == args.end()) throw std::invalid_argument("Word is required for diff query.");
            
            idx1 = new VersionedIndexer(args["version1"]);
            idx1->processFile(args["file1"], buffer_size_kb);
            
            idx2 = new VersionedIndexer(args["version2"]); 
            idx2->processFile(args["file2"], buffer_size_kb);

            processor = new DiffQuery(*idx1,*idx2, args["word"]);


        }else {
            invalid_argument("Invalid Query type");
        }


        processor->execute();
        
        auto end_time = chrono:: high_resolution_clock::now();
        chrono::duration<double> dur = end_time-start_time;


        cout << "Buffer size (KB) : " << buffer_size_kb << endl;
        cout << "Execution time (s): " <<  dur.count() << endl;

        delete idx1;
        delete idx2;


    } catch (const exception& e){
        std::cerr << "ERROR: " << e.what() << endl;
        return 1;
    }

    return 0;
}


