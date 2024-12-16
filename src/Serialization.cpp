#include "Serialization.h"


bool read_string(const SKSE::SerializationInterface* a_intfc, std::string& a_str)
{
    std::vector<std::pair<int, bool>> encodedStr;
	std::size_t size;
    if (!a_intfc->ReadRecordData(size)) {
        return false;
    }
    for (std::size_t i = 0; i < size; i++) {
        std::pair<int, bool> temp_pair;
        if (!a_intfc->ReadRecordData(temp_pair)) {
			return false;
		}
        encodedStr.push_back(temp_pair);
	}
    a_str = decodeString(encodedStr);
	return true;
}

bool write_string(const SKSE::SerializationInterface* a_intfc, const std::string& a_str)
{
    const auto encodedStr = encodeString(a_str);
    // i first need the size to know no of iterations
    if (const auto size = encodedStr.size(); !a_intfc->WriteRecordData(size)) return false;
    for (const auto& temp_pair : encodedStr) {
        if (!a_intfc->WriteRecordData(temp_pair)) {
            return false;
        }
    }
    return true;
};