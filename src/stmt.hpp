#ifndef STMT_HPP
#define STMT_HPP

#include <mutex>
#include <string>
#include <vector>

#include <boost/container/small_vector.hpp>

#include "array_pool.hpp"
#include "ir_decl.hpp"
#include "handle.hpp"
#include "parser_types.hpp"
#include "pstring.hpp"

// stmts form a crude IR that is generated by the parser and 
// sent to the SSA IR builder.

namespace bc = ::boost::container;

using stmt_ht = handle_t<unsigned, struct stmt_ht_tag, ~0>;

#define STMT_XENUM \
    X(STMT_END_BLOCK)\
    X(STMT_EXPR)\
    X(STMT_IF)\
    X(STMT_ELSE)\
    X(STMT_WHILE)\
    X(STMT_DO)\
    X(STMT_RETURN)\
    X(STMT_BREAK)\
    X(STMT_CONTINUE)\
    X(STMT_LABEL)\
    X(STMT_GOTO) \
    X(STMT_GOTO_MODE)

// Negative values represent var inits, where the negated value 
// holds the bitwise negated index of the fn variable.
// (See 'get_local_var_i')
enum stmt_name_t : int
{
    STMT_MIN_VAR_DECL = INT_MIN,
    STMT_MAX_VAR_DECL = -1,
#define X(x) x,
    STMT_XENUM
#undef X
};

std::string to_string(stmt_name_t);

constexpr bool is_var_init(stmt_name_t stmt_name)
{
    return stmt_name < STMT_END_BLOCK;
}

constexpr unsigned get_local_var_i(stmt_name_t stmt_name)
{
    assert(is_var_init(stmt_name));
    return ~static_cast<unsigned>(stmt_name);
}

struct label_t
{
    cfg_ht node;
    stmt_ht stmt_h;
    unsigned goto_count;
    bc::small_vector<cfg_ht, 2> inputs;
};

struct stmt_t
{
    stmt_name_t name;
    pstring_t pstring;
    union
    {
        token_t const* expr;
        label_t* label;
    };

    // You can permanently allocate exprs and labels with these:
    static token_t const* new_expr(token_t const* begin, token_t const* end);
    static label_t* new_label();

private:
    inline static std::mutex label_pool_mutex;
    inline static array_pool_t<label_t> label_pool;

    inline static std::mutex expr_pool_mutex;
    inline static array_pool_t<token_t> expr_pool;
};

std::string to_string(stmt_name_t stmt_name);

// Represents function data right after parsing
class fn_def_t
{
public:
    unsigned num_params = 0;
    // First elems are params
    std::vector<var_decl_t> local_vars;
    std::vector<stmt_t> stmts;

    stmt_t const& operator[](stmt_ht h) const { return stmts[h.value]; }
    stmt_t& operator[](stmt_ht h) { return stmts[h.value]; }

    stmt_ht next_stmt() const { return { stmts.size() }; }

    stmt_ht push_stmt(stmt_t stmt) 
    { 
        stmt_ht handle = next_stmt();
        stmts.push_back(stmt); 
        return handle;
    }

    stmt_ht push_var_init(unsigned name, token_t const* expr)
    { 
        stmt_ht handle = next_stmt();
        stmts.push_back({ static_cast<stmt_name_t>(~name), {}, expr }); 
        return handle;
    }
};

#endif
