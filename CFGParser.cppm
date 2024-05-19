module;

export module CFGParser;

import <functional>;
import <string>;
import <vector>;
import <unordered_map>;
import <iostream>;
import <fstream>;
import <array>;
import <concepts>;


export template<std::integral T>
struct vec2 final
{
	T x{ static_cast<T>(0) };
	T y{ static_cast<T>(0) };
};

export template<std::integral T>
struct vec3 final
{
	T x{ static_cast<T>(0) };
	T y{ static_cast<T>(0) };
	T z{ static_cast<T>(0) };
};

export template<std::integral T>
struct vec4 final
{
	T x{ static_cast<T>(0) };
	T y{ static_cast<T>(0) };
	T z{ static_cast<T>(0) };
	T w{ static_cast<T>(0) };
};

/**
	\brief Config parser class.
	Config file syntax:
   >section<       >inheritance<              >attibutes<
	[name] : base_struct0, base_struct1 = attribute0, attribute1
	key = value
	array = 6356, 52, 234, 45345
	string = "Some of something"
	multistr = "You can use\n
		 multiline \n
		string too!"
	;this is comment line!
	|And this is comment block
	which you can use as multiline|
*/
export class CFGParser final
{
public:

	using ValueHash = std::unordered_map<std::string, std::string>;

	struct Section final
	{
		std::vector<std::string> inheritances;
		std::vector<std::string> attributes;
		ValueHash values;
	};

	using SectionDataHash = std::unordered_map<std::string, Section>;

private:
	SectionDataHash _section_data;

	static std::function<void(const std::string&)> _msg_functor;

	static constexpr char comment_character {';'};
	static constexpr char comment_multiline {'|'};

	std::string _current_file;
	std::string _base_path;

	// Some dummies for return unexistance things
	const std::vector<std::string> _dummy;
	const std::string _dummy_str;

	static constexpr std::array<char, 63u> allowed_characters
	{
		'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
		'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
		'_'
	};

public:
	CFGParser()
	{
		this->setDefaultCallback();
	}

	CFGParser(const std::string& file_path)
	{
		this->setDefaultCallback();
		this->load(file_path);
	}

	~CFGParser() = default;

	// We cannot copy config file or move it. Sorry)
	CFGParser(CFGParser const&) = delete;
	CFGParser(CFGParser const&&) = delete;
	CFGParser const& operator=(CFGParser const&) = delete;
	CFGParser const& operator=(CFGParser const&&) = delete;

	/**
		\brief Sets include base path.
	*/
	void setBasePath(const std::string& path) { _base_path = path; }

	/**
		\brief You can use your own message functor.
	*/
	template<typename F> static void setMessageFunctor(F&& func) { _msg_functor = std::move(func); }

	/**
		\brief Load and parse config file.
	*/
	void load(const std::string& file_path)
	{
		_current_file = file_path;

		std::ifstream file(_current_file);

		if (!file.good())
		{
			if (_msg_functor)
				_msg_functor("Cannot open file \"" + file_path + "\".");

			return;
		}

		std::string section, inheritance, attribute, key, value;
		std::pair<std::string, std::string> preprocessor_pair;

		Section* section_ptr = nullptr;

		enum class ParseAction : uint8_t
		{
			NEW_LINE = 0u,
			SECTION,
			INHERITANCE,
			ATTRIBUTE,
			KEY,
			VALUE,
			VALUE_ARRAY,
			STRING_VALUE,
			COMMENT,
			MULTILINE_COMMENT,
			PREPROCESSOR,
			INCLUDE,
			ERROR
		};

		ParseAction parse_action = ParseAction::NEW_LINE;

		uint32_t line = 1u, character_pos = 0u;
		bool ignore_current_spaces = true;

		const auto GetErrorLineString = [&line, &character_pos]() -> std::string
		{
			return "Error at line \'" + std::to_string(line) +
				"\', character at \'" + std::to_string(character_pos) + "\' : ";
		};

		const auto msg = [&GetErrorLineString, this](const std::string& message) -> void
		{
			if (_msg_functor)
				_msg_functor(GetErrorLineString() + message);
		};

		const auto PushInheritance = [&]() -> void
		{
			if (!inheritance.empty() && (section_ptr != nullptr))
			{
				if (_section_data.find(inheritance) != _section_data.cend())
					section_ptr->inheritances.push_back(inheritance);
				else
					msg("Inherited section \"" + inheritance + "\" is not exist!");

				inheritance.clear();
			}
		};

		const auto PushAttribute = [&]() -> void
		{
			if (!attribute.empty() && (section_ptr != nullptr))
			{
				section_ptr->attributes.push_back(attribute);
				attribute.clear();
			}
		};

		const auto ProcessPreprocessor = [&]() -> void
		{
			preprocessor_pair.first.clear();
			preprocessor_pair.second.clear();
		};

		const auto IncludeFile = [&](const std::string& path) -> void
		{
			const auto file = _current_file;
			this->load(_base_path + path);
			_current_file = file;
		};

		while (!file.eof())
		{
			auto character = file.get();

			// eof
			if (character < 0)
				character = '\n';

			switch (character)
			{
				case comment_character:
				{
					switch (parse_action)
					{
						case ParseAction::STRING_VALUE:
							value += character;
						break;

						default:
							parse_action = ParseAction::COMMENT;
						break;
					}
				}
				break;

				case comment_multiline:
				{
					switch (parse_action)
					{
						case ParseAction::STRING_VALUE:
							value += character;
						break;

						case ParseAction::MULTILINE_COMMENT:
							parse_action = ParseAction::NEW_LINE;
						break;

						default:
							parse_action = ParseAction::MULTILINE_COMMENT;
						break;
					}
				}
				break;

				case ' ':
				case '\t':
				{
					switch (parse_action)
					{
						case ParseAction::STRING_VALUE:
							value += character;
						break;

						case ParseAction::PREPROCESSOR:
						{
							if (preprocessor_pair.first == "include")
								parse_action = ParseAction::INCLUDE;

							preprocessor_pair.first.clear();
						}
						break;

						case ParseAction::ATTRIBUTE:
						case ParseAction::INHERITANCE:
						case ParseAction::KEY:
						case ParseAction::SECTION:
						case ParseAction::VALUE:
						{
							if (!ignore_current_spaces)
							{
								parse_action = ParseAction::ERROR;
								msg("Space in wrong place");
							}
						}
						break;

						default:
						break;
					}
				}
				break;

				case '\\':
				{
					switch (parse_action)
					{
						case ParseAction::COMMENT:
						case ParseAction::MULTILINE_COMMENT:
						break;

						case ParseAction::STRING_VALUE:
						{
							switch (file.get())
							{
								case '\\':
									value += '\\';
								break;

								case 'n':
									value += '\n';
								break;

								case '\"':
									value += '\"';
								break;

								case '\'':
									value += '\'';
								break;

								default:
									msg("Unknown escape-sequence symbol");
								break;
							}
						}
						break;

						default:
							parse_action = ParseAction::ERROR;
							msg("Unexpected escape-symbol");
						break;
					}
				}
				break;

				case '\"':
				{
					switch (parse_action)
					{
						case ParseAction::STRING_VALUE:
							parse_action = ParseAction::VALUE;
						break;

						case ParseAction::VALUE:
							parse_action = ParseAction::STRING_VALUE;
						break;

						default:
						break;
					}
				}
				break;

				case '#':
				{
					switch (parse_action)
					{
						case ParseAction::COMMENT:
						case ParseAction::MULTILINE_COMMENT:
						break;

						case ParseAction::NEW_LINE:
						{
							parse_action = ParseAction::PREPROCESSOR;
						}
						break;

						case ParseAction::STRING_VALUE:
							value += character;
						break;

						default:
							parse_action = ParseAction::ERROR;
							msg("Preprocessor parse error");
						break;
					}
				}
				break;

				case '\n':
				{
					switch (parse_action)
					{
						case ParseAction::COMMENT:
						case ParseAction::MULTILINE_COMMENT:
						break;

						case ParseAction::INHERITANCE:
							PushInheritance();
						break;

						case ParseAction::PREPROCESSOR:
						case ParseAction::INCLUDE:
						break;

						case ParseAction::ATTRIBUTE:
							PushAttribute();
						break;

						case ParseAction::VALUE:
						case ParseAction::VALUE_ARRAY:
						{
							if (section_ptr != nullptr)
								section_ptr->values[key] = value;

							key.clear();
							value.clear();
						}
						break;

						case ParseAction::STRING_VALUE:
						case ParseAction::NEW_LINE:
						break;

						case ParseAction::SECTION:
							parse_action = ParseAction::NEW_LINE;
						break;

						default:
							parse_action = ParseAction::ERROR;
							msg("New line parse error");
						break;
					}

					if ((parse_action != ParseAction::STRING_VALUE) &&
						(parse_action != ParseAction::MULTILINE_COMMENT))
					{
						parse_action = ParseAction::NEW_LINE;
						character_pos = 0u;
					}

					++line;
					ignore_current_spaces = true;
				}
				break;

				case '<':
				{
					switch (parse_action)
					{
						case ParseAction::STRING_VALUE:
							value += character;
						break;

						case ParseAction::INCLUDE:
						break;

						default:
						break;
					}
				}
				break;

				case '>':
				{
					switch (parse_action)
					{
						case ParseAction::STRING_VALUE:
							value += character;
						break;

						case ParseAction::INCLUDE:
						{
							IncludeFile(preprocessor_pair.second);
							preprocessor_pair.second.clear();
						}
						break;

						default:
						break;
					}
				}
				break;

				case '[':
				{
					switch (parse_action)
					{
						case ParseAction::COMMENT:
						case ParseAction::MULTILINE_COMMENT:
						break;

						case ParseAction::NEW_LINE:
						{
							ignore_current_spaces = false;
							parse_action = ParseAction::SECTION;
							section.clear();
							section_ptr = nullptr;
						}
						break;

						case ParseAction::STRING_VALUE:
							value += character;
						break;

						default:
							parse_action = ParseAction::ERROR;
							msg("Section naming parse error");
						break;
					}
				}
				break;

				case ']':
				{
					switch (parse_action)
					{
						case ParseAction::COMMENT:
						case ParseAction::MULTILINE_COMMENT:
						break;

						case ParseAction::SECTION:
						{
							const auto& pair = _section_data.try_emplace(section, Section{});

							if (pair.second)
								section_ptr = &pair.first->second;
							else
								msg("Section \"" + section + "\" already exist.");

							ignore_current_spaces = true;
						}
						break;

						case ParseAction::STRING_VALUE:
							value += character;
						break;

						default:
							parse_action = ParseAction::ERROR;
							msg("Section naming parse error");
						break;
					}
				}
				break;

				case ',':
				{
					switch (parse_action)
					{
						case ParseAction::COMMENT:
						case ParseAction::MULTILINE_COMMENT:
						break;

						case ParseAction::INHERITANCE:
							PushInheritance();
						break;

						case ParseAction::ATTRIBUTE:
							PushAttribute();
						break;

						case ParseAction::STRING_VALUE:
						case ParseAction::VALUE_ARRAY:
							value += character;
						break;

						case ParseAction::VALUE:
							parse_action = ParseAction::VALUE_ARRAY;
							value += character;
						break;

						default:
							parse_action = ParseAction::ERROR;
							msg("Enumeration error");
						break;
					}
				}
				break;

				case ':':
				{
					switch (parse_action)
					{
						case ParseAction::COMMENT:
						case ParseAction::MULTILINE_COMMENT:
						break;

						case ParseAction::SECTION:
							parse_action = ParseAction::INHERITANCE;
						break;

						case ParseAction::STRING_VALUE:
							value += character;
						break;

						default:
							parse_action = ParseAction::ERROR;
							msg("Inheritance error");
						break;
					}
				}
				break;

				case '=':
				{
					switch (parse_action)
					{
						case ParseAction::COMMENT:
						case ParseAction::MULTILINE_COMMENT:
						break;

						case ParseAction::SECTION:
							parse_action = ParseAction::ATTRIBUTE;
						break;

						case ParseAction::INHERITANCE:
						{
							PushInheritance();

							parse_action = ParseAction::ATTRIBUTE;
						}
						break;

						case ParseAction::KEY:
						{
							if (section_ptr != nullptr)
							{
								const auto& pair = section_ptr->values.try_emplace(key, std::string {});

								if (!pair.second)
									msg("Section \"" + section + "\" key \"" + key + "\" already exist.");
							}

							parse_action = ParseAction::VALUE;
						}
						break;

						case ParseAction::STRING_VALUE:
							value += character;
						break;

						default:
							parse_action = ParseAction::ERROR;
							msg("Set value error");
						break;
					}
				}
				break;

				default:
				{
					switch (parse_action)
					{
						case ParseAction::COMMENT:
						case ParseAction::MULTILINE_COMMENT:
						break;

						case ParseAction::NEW_LINE:
						{
							parse_action = ParseAction::KEY;
							key += character;
						}
						break;

						case ParseAction::PREPROCESSOR:
							preprocessor_pair.first += character;
						break;

						case ParseAction::INCLUDE:
							preprocessor_pair.second += character;
						break;

						case ParseAction::SECTION:
							section += character;
						break;

						case ParseAction::INHERITANCE:
							inheritance += character;
						break;

						case ParseAction::ATTRIBUTE:
							attribute += character;
						break;

						case ParseAction::KEY:
							key += character;
						break;

						case ParseAction::VALUE:
						case ParseAction::VALUE_ARRAY:
							value += character;
						break;

						case ParseAction::STRING_VALUE:
							value += character;
						break;

						default:
							parse_action = ParseAction::ERROR;
							msg("Invalid character error");
						break;
					}
				}
				break;
			}

			++character_pos;
		}

		file.close();
	}

	void save(const std::string& file_path)
	{
		std::ofstream file(file_path);

		for (const auto& pair : _section_data)
		{
			file << '[' << pair.first << ']';

			if (!pair.second.inheritances.empty())
			{
				file << " : ";

				for (auto iter = pair.second.inheritances.cbegin();
					iter != pair.second.inheritances.cend(); ++iter)
				{
					if (iter != pair.second.inheritances.cbegin())
						file << ", ";

					file << (*iter);
				}
			}

			if (!pair.second.attributes.empty())
			{
				file << " = ";

				for (auto iter = pair.second.attributes.cbegin();
					iter != pair.second.attributes.cend(); ++iter)
				{
					if (iter != pair.second.attributes.cbegin())
						file << ", ";

					file << (*iter);
				}
			}

			file << '\n';

			for (const auto& pair : pair.second.values)
				file << pair.first << " = " << pair.second << '\n';

			file << '\n';
		}

		file.close();
	}

	void saveCurrent() { this->save(_current_file); }

	/**
		\brief Checking that the section have some attributes(string flags).
	*/
	const bool hasAttribute(const std::string& section, const std::string& attribute) const
	{
		if (const auto iter = _section_data.find(section); iter != _section_data.cend())
		{
			for (const auto& section_attribute : iter->second.attributes)
			{
				if (section_attribute == attribute)
					return true;
			}
		}

		return false;
	}

	const bool hasAttributes(const std::string& section) const
	{
		if (const auto iter = _section_data.find(section);
			iter != _section_data.cend())
		{
			return (!iter->second.attributes.empty());
		}
		else
		{
			if (_msg_functor)
				_msg_functor("Section \"" + section + "\" is not exist!");
		}

		return false;
	}

	const std::vector<std::string>& getAttributes(const std::string& section) const
	{
		if (const auto iter = _section_data.find(section);
			iter != _section_data.cend())
		{
			return iter->second.attributes;
		}
		else
		{
			if (_msg_functor)
				_msg_functor("Section \"" + section + "\" is not exist!");
		}

		return _dummy;
	}

	/**
		\brief Checking is section exists.
	*/
	const bool hasSection(const std::string& section) const
	{
		return (_section_data.find(section) != _section_data.cend());
	}

	/**
		\brief Checking is key exist inside a section.
	*/
	const bool hasKey(const std::string& section, const std::string& key) const
	{
		if (const auto iter = _section_data.find(section);
			iter != _section_data.cend())
		{
			return (iter->second.values.find(key) != iter->second.values.cend());
		}
		else
		{
			if (_msg_functor)
				_msg_functor("Section \"" + section + "\" is not exist!");
		}

		return false;
	}

	/**
		\brief Checking is section has inheritance from some other section.
	*/
	const bool isInheritedFrom(const std::string& section, const std::string& base_section) const
	{
		if (const auto iter = _section_data.find(section);
			iter != _section_data.cend())
		{
			for (const auto& inherited : iter->second.inheritances)
			{
				if (inherited == base_section)
					return true;
			}
		}

		return false;
	}

	const bool hasInheritances(const std::string& section) const
	{
		if (const auto iter = _section_data.find(section);
			iter != _section_data.cend())
		{
			return (!iter->second.inheritances.empty());
		}
		else
		{
			if (_msg_functor)
				_msg_functor("Section \"" + section + "\" is not exist!");
		}

		return false;
	}

	const std::vector<std::string>& getInheritances(const std::string& section) const
	{
		if (const auto iter = _section_data.find(section);
			iter != _section_data.cend())
		{
			return iter->second.inheritances;
		}
		else
		{
			if (_msg_functor)
				_msg_functor("Section \"" + section + "\" is not exist!");
		}

		return _dummy;
	}

	/**
		\brief Get string from config file.
	*/
	const std::string& getString(const std::string& section, const std::string& key, const std::string& default_value = {}) const
	{
		if (const auto section_iter = _section_data.find(section);
			section_iter != _section_data.cend())
		{
			const auto& values = section_iter->second.values;

			if (const auto value_iter = values.find(key);
				value_iter != values.cend())
				return value_iter->second;

			// So... If we found value in inherited section only - we return it.
			// But! If we have same keys inside all inherited sections?
			// Hmm... Should we return a first value? Well, let it be for now.
			// So, inheritance priority will be...
			// [section] : higher, middle, lower
			if (const auto& value = this->getValueFromInheritance(section_iter->second, key); !value.empty())
				return value;

			return default_value;
		}
		else
		{
			return default_value;
		}
	}

	/**
		\brief Parse value to desired type. Important! Do not set type as string!
	*/
	template<typename T>
	inline const T get(const std::string& section, const std::string& key, const T& default_value = static_cast<T>(0)) const
	{
		const auto& str = this->getString(section, key);

		if (!str.empty())
			return makeValueFromString<T>(str);
		else
			return default_value;
	}

	template<typename T>
	inline void set(const std::string& section, const std::string& key, const T value)
	{
		if (const auto iter = _section_data.find(section);
			iter != _section_data.cend())
		{
			if (iter->second.values.find(key) != iter->second.values.cend())
			{
				iter->second.values[key] = std::to_string(value);
			}
			else
			{
				if (_msg_functor)
					_msg_functor("Section \"" + section + "\" key \"" + key + "\" is not exist!");
			}
		}
		else
		{
			if (_msg_functor)
				_msg_functor("Section \"" + section + "\" is not exist!");
		}
	}

	/**
		\brief Get array value.
	*/
	template<typename T>
	inline const std::vector<T> getArray(const std::string& section, const std::string& key) const
	{
		const auto& str = this->getString(section, key);

		if (!str.empty())
		{
			std::vector<T> result;
			std::string num_str;
			const size_t str_len = str.length();

			for (const auto character :  str)
			{
				if (character == ',')
				{
					result.push_back(makeValueFromString<T>(num_str));
					num_str.clear();
				}
				else
				{
					num_str += character;
				}
			}

			// push last element
			result.push_back(makeValueFromString<T>(num_str));

			return result;
		}
		else
		{
			return {};
		}
	}

	/**
		\brief Returns section number.
	*/
	const size_t getSectionCount() const { return _section_data.size(); }

	/**
		\brief Returns all cfg data reference.
	*/
	const SectionDataHash& getSectionData() const { return _section_data; }

	template<std::integral T>
	vec2<T> getVec2(const std::string& section, const std::string& key, const vec2<T>& default_value = {}) const
	{
		if (const auto& vec{ this->getArray<T>(section, key) }; !vec.empty())
		{
			return { vec[0u], vec[1u] };
		}
		else
		{
			_msg_functor("Cannot find \"" + section + ':' + key + "\". Default value will be return...");

			return default_value;
		}
	}

	template<std::integral T>
	vec3<T> getVec3(const std::string& section, const std::string& key, const vec3<T>& default_value = {}) const
	{
		if (const auto& vec{ this->getArray<T>(section, key) }; !vec.empty())
		{
			return { vec[0u], vec[1u], vec[2u] };
		}
		else
		{
			_msg_functor("Cannot find \"" + section + ':' + key + "\". Default value will be return...");

			return default_value;
		}
	}

	template<std::integral T>
	vec4<T> getVec4(const std::string& section, const std::string& key, const vec4<T>& default_value = {}) const
	{
		if (const auto& vec{ this->getArray<T>(section, key) }; !vec.empty())
		{
			return { vec[0u], vec[1u], vec[2u], vec[3u] };
		}
		else
		{
			_msg_functor("Cannot find \"" + section + ':' + key + "\". Default value will be return...");

			return default_value;
		}
	}

private:
	template<typename T>
	inline constexpr T makeValueFromString(const std::string& string_value) const
	{
		T value = static_cast<T>(0);

		if constexpr (std::is_same<T, bool>::value)
		{
			if (string_value == "true" || string_value == "on" || string_value == "yes")
				value = true;
		}
		else if constexpr (std::is_same<T, int>::value)
		{
			value = std::stoi(string_value);
		}
		else if constexpr (std::is_same<T, uint32_t>::value)
		{
			value = std::stoul(string_value);
		}
		else if constexpr (std::is_same<T, float>::value)
		{
			value = std::stof(string_value);
		}
		else if constexpr (std::is_same<T, double>::value)
		{
			value = std::stod(string_value);
		}
		else if constexpr (std::is_same<T, long double>::value)
		{
			value = std::stold(string_value);
		}
		else if constexpr (std::is_same<T, int64_t>::value)
		{
			value = std::stoll(string_value);
		}
		else if constexpr (std::is_same<T, uint64_t>::value)
		{
			value = std::stoull(string_value);
		}
		else
		{
			value = static_cast<T>(std::stoi(string_value));
		}

		return value;
	}

	const std::string& getValueFromInheritance(const Section& section_data, const std::string& key) const
	{
		for (const auto& inheritance : section_data.inheritances)
		{
			if (const auto iter = _section_data.find(inheritance);
				iter != _section_data.cend())
			{
				if (const auto key_iter = iter->second.values.find(key);
					key_iter != iter->second.values.cend())
				{
					return key_iter->second;
				}
			}
		}

		return _dummy_str;
	}

	inline constexpr bool isCharacterValid(const char character)
	{
		return std::find(
			allowed_characters.cbegin(),
			allowed_characters.cend(),
			character) == allowed_characters.cend();
	}

	inline void setDefaultCallback()
	{
		if (!_msg_functor)
		{
			_msg_functor = std::move([](const std::string& msg)
				{
					std::cout << "CFGParser: " << msg << std::endl;
				});
		}
	}

};
