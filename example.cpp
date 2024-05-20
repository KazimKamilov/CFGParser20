import <iostream>;
import <sstream>;
import <vector>;

import CFGParser;

template<typename T>
std::string asStrArray(const std::vector<T>& arr)
{
    std::ostringstream stream;

    for (const auto& s : arr)
        stream << s << ", ";

    return stream.str();
}

int main()
{
    CFGParser cfg("test.cfg");

    const auto& vec{ cfg.getVec2<int>("parent", "vec") };
    const auto& str{ cfg.getString("name", "multistr") };
    const auto& arr{ cfg.getArray<int>("name", "array") };
    const auto& attribs{ cfg.getAttributes("name") };

    std::cout << "vec is: " << vec.x << ", " << vec.y << std::endl;
    std::cout << "multistring is: " << str << std::endl;
    std::cout << "array is: " << asStrArray(arr) << std::endl;
    std::cout << "attribs is: " << asStrArray(attribs) << std::endl;
}
