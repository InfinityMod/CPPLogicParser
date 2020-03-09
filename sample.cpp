#include "logic_parser.h"

void printParse(std::map<std::string, std::set<std::string>> valueMap, std::string logic_str)
{
    std::cout << "Parse: " << logic_str << std::endl;

    auto lp = LogicParser_Set<std::string>(valueMap, logic_str);
    auto new_set = lp.parse();

    bool start = true;
    std::cout << "Result: (";
    for(auto && s: new_set)
    {   
        if (start){
            std::cout << s;
            start = false;
        }else{
            std::cout << ", " << s;
        }
    }
    std::cout << ");"<<std::endl<<std::endl;;
}

int main(int argc, char** argv) 
{
    std::map<std::string, std::set<std::string>> valueMap = {
        {"a", {"ab", "ba", "cc"} },
        {"b", {"abc", "bac", "cc"} }
    };

    std::cout << "a: (ab, ba, cc)" << std::endl;
    std::cout << "b: (abc, bac, cc)" << std::endl;
    std::cout << std::endl;

    //Logical And
    printParse(valueMap, "a&b");
    

    //Logical OR
    printParse(valueMap, "a|b");

    //Logical NOT
    printParse(valueMap, "a ! b");

    //Logical NOTOR
    printParse(valueMap, "(a ! b) | (b ! a)");

    //Logical NOTOR
    printParse(valueMap, "(a ! b) & (b ! a)");

    if (argc > 1)
    {
        printParse(valueMap, "argv[1]");
    }    
    return 0; 
}