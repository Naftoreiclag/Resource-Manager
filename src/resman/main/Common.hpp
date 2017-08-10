#ifndef RESMAN_MAIN_COMMON_HPP
#define RESMAN_MAIN_COMMON_HPP

namespace resman {

typedef std::string OType;

/**
 * @class Object
 * @brief A single resource to be translated (or not)
 */
struct Object {
    std::string m_name;
    OType m_type;
    boost::filesystem::path m_src_file;
    uint32_t m_src_hash;
    
    Json::Value m_params;
    uint32_t m_params_hash;
    
    bool m_skip_retrans = false;
    bool m_force_retrans = false;
    
    bool m_expanded = false;
    
    boost::filesystem::path m_interm_file;

    boost::filesystem::path m_dest_file;
    uint32_t m_dest_size;
    
    boost::filesystem::path m_dbg_resdef;
};

} // namespace resman

#endif // RESMAN_MAIN_COMMON_HPP
