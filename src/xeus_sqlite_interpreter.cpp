/***************************************************************************
* Copyright (c) 2020, Mariana Meireles                                     *
* Copyright (c) 2020, QuantStack                                           *
*                                                                          *
* Distributed under the terms of the BSD 3-Clause License.                 *
*                                                                          *
* The full license is in the file LICENSE, distributed with this software. *
****************************************************************************/

#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <memory>
#include <stack>
#include <cctype>
#include <cstdio>

#include "xeus/xinterpreter.hpp"

#include "xeus-sqlite/xeus_sqlite_interpreter.hpp"

#include <SQLiteCpp/SQLiteCpp.h>
#include <SQLiteCpp/VariadicBind.h>

namespace xeus_sqlite
{

std::string interpreter::sanitize_string(const std::string& code)
{
    /*
        Cleans the code from inputs that are acceptable in a jupyter notebook.
    */
    std::string aux = code;
    aux.erase(
        std::remove_if(
            aux.begin(), aux.end(), [](char const c) {
                return '\n' == c || '\r' == c || '\0' == c || '\x1A' == c;
            }
        ),
        aux.end()
    );
    return aux;
}

std::vector<std::string> interpreter::tokenizer(const std::string& code)
{
    /*
        Separetes the code on spaces so it's easier to execute the commands.
    */
    std::stringstream input(sanitize_string(code));
    std::string segment;
    std::vector<std::string> tokenized_str;
    std::string is_magic(1, input.str()[0]);
    tokenized_str.push_back(is_magic);

    while(std::getline(input, segment, ' '))
    {
        tokenized_str.push_back(segment);
    }


    return tokenized_str;
}

bool interpreter::is_magic(std::vector<std::string>& tokenized_code)
{
    /*
        Returns true if the code input is magic and false if isn't.
    */
    if(tokenized_code[0] == "%")
    {
        tokenized_code[1].erase(0, 1);
        std::transform(tokenized_code[1].begin(), tokenized_code[1].end(),
                        tokenized_code[1].begin(), ::toupper);
        return true;
    }
    else
    {
        return false;
    }
}

void interpreter::load_db(const std::vector<std::string> tokenized_code)
{
    /*
        Loads the a database. If the open mode is not specified it defaults
        to read and write mode.
    */
    m_bd_is_loaded = true;

    try
    {
        if (tokenized_code.back().find("rw") != std::string::npos)
        {
            m_db = std::make_unique<SQLite::Database>(m_db_path,
                        SQLite::OPEN_READWRITE);
        }
        else if (tokenized_code.back().find("r") != std::string::npos)
        {
            m_db = std::make_unique<SQLite::Database>(m_db_path,
                        SQLite::OPEN_READONLY);
        }
        //Opening as read and write because mode is unspecified
        else if (tokenized_code.size() < 2)
        {
            m_db = std::make_unique<SQLite::Database>(m_db_path,
                        SQLite::OPEN_READWRITE);
        }
    }
    catch (const std::runtime_error& err)
    {
        nl::json jresult;
        jresult["status"] = "error";
        jresult["ename"] = "Error";
        jresult["evalue"] = err.what();
        publish_stream("stderr", err.what());
    }
}

void interpreter::create_db(const std::vector<std::string> tokenized_code)
{
    /*
        Creates the a database in read and write mode.
    */
    m_bd_is_loaded = true;

    try
    {
        m_db_path = tokenized_code[2];
        m_db_path += tokenized_code[3];
        m_db = std::make_unique<SQLite::Database>(m_db_path,
                        SQLite::OPEN_READWRITE|SQLite::OPEN_CREATE);
    }
    catch(const std::runtime_error& err)
    {
        nl::json jresult;
        jresult["status"] = "error";
        jresult["ename"] = "Error";
        jresult["evalue"] = err.what();
        publish_stream("stderr", err.what());
    }
}

void interpreter::delete_db()
{
    /*
        Deletes the database.
    */
    m_bd_is_loaded = false;

    if(std::remove(m_db_path.c_str()) != 0)
    {
        publish_stream("stderr", "Error deleting file.\n");
    }
    else
    {
        publish_stream("stderr", "File successfully deleted.\n");
    }
}

void interpreter::table_exists(const std::string table_name)
{
    if (m_db->SQLite::Database::tableExists(table_name.c_str()))
    {
        publish_stream("stdout", "The table " + table_name + " exists.\n");
    }
    else
    {
        publish_stream("stdout", "The table " + table_name + " doesn't exist.\n");
    }
}

void interpreter::is_unencrypted()
{
    if (SQLite::Database::isUnencrypted(m_db_path))
    {
        publish_stream("stdout", "The database is unencrypted.\n");
    }
    else
    {
        publish_stream("stdout", "The database is encrypted.\n");
    }
}

void interpreter::get_header_info()
{
    SQLite::Header header;
    header = SQLite::Database::getHeaderInfo(m_db_path);

   // Official documentation for fields can be found here: https://www.sqlite.org/fileformat.html#the_database_header
    std::string str(header.headerStr);
    std::cout << header.headerStr << std::endl;
    // publish_stream("stdout", "Magic header string: " + (str)header.headerStr + "\n");
    // publish_stream("stdout", "Page size bytes: " + header.pageSizeBytes + "\n");
    // publish_stream("stdout", "File format write version: " + (int)header.fileFormatWriteVersion + "\n");
    // publish_stream("stdout", "File format read version: " + (int)header.fileFormatReadVersion + "\n");
    // publish_stream("stdout", "Reserved space bytes: " + (int)header.reservedSpaceBytes + "\n");
    // publish_stream("stdout", "Max embedded payload fraction " + (int)header.maxEmbeddedPayloadFrac + "\n");
    // publish_stream("stdout", "Min embedded payload fraction: " + (int)header.minEmbeddedPayloadFrac + "\n");
    // publish_stream("stdout", "Leaf payload fraction: " << (int)header.leafPayloadFrac + "\n");
    // publish_stream("stdout", "File change counter: " + header.fileChangeCounter + "\n");
    // publish_stream("stdout", "Database size pages: " + header.databaseSizePages + "\n");
    // publish_stream("stdout", "First freelist trunk page: " + header.firstFreelistTrunkPage + "\n");
    // publish_stream("stdout", "Total freelist trunk pages: " + header.totalFreelistPages + "\n");
    // publish_stream("stdout", "Schema cookie: " + header.schemaCookie + "\n");
    // publish_stream("stdout", "Schema format number: " + header.schemaFormatNumber + "\n");
    // publish_stream("stdout", "Default page cache size bytes: " + header.defaultPageCacheSizeBytes + "\n");
    // publish_stream("stdout", "Largest B tree page number: " + header.largestBTreePageNumber + "\n");
    // publish_stream("stdout", "Database text encoding: " + header.databaseTextEncoding + "\n");
    // publish_stream("stdout", "User version: " + header.userVersion + "\n");
    // publish_stream("stdout", "Incremental vaccum mode: " + header.incrementalVaccumMode + "\n");
    // publish_stream("stdout", "Application ID: " + header.applicationId + "\n");
    // publish_stream("stdout", "Version valid for: " + header.versionValidFor + "\n");
    // publish_stream("stdout", "SQLite version: " + header.sqliteVersion + "\n");
}

void interpreter::parse_code(const std::vector<std::string>& tokenized_code)
{
    std::cout << "You're using magic. " << std::endl;

    if (tokenized_code[1] == "LOAD" || tokenized_code[1] == "CREATE")
    {
        m_db_path = tokenized_code[2];

        std::ifstream path_is_valid(m_db_path);
        if (!path_is_valid.is_open())
        {
          publish_stream("stderr", "The path doesn't exist.\n");
        }
        else
        {
            if (tokenized_code[1] == "LOAD")
            {
                load_db(tokenized_code);
            }
            else if (tokenized_code[1] == "CREATE")
            {
                create_db(tokenized_code);
            }
        }
    }

    else if (m_bd_is_loaded)
    {
        if (tokenized_code[1] == "DELETE")
        {
            delete_db();
        }
        else if (tokenized_code[1] == "TABLE_EXISTS")
        {
            table_exists(tokenized_code[2]);
        }
        else if (tokenized_code[1] == "LOAD_EXTENSION")
        {
            m_db->SQLite::Database::loadExtension(tokenized_code[2].c_str(),
                    tokenized_code[3].c_str());
        }
        else if (tokenized_code[1] == "SET_KEY")
        {
            m_db->SQLite::Database::key(tokenized_code[2]);
        }
        else if (tokenized_code[1] == "REKEY")
        {
            m_db->SQLite::Database::rekey(tokenized_code[2]);
        }
        else if (tokenized_code[1] == "IS_UNENCRYPTED")
        {
            is_unencrypted();
        }
        else if (tokenized_code[1] == "GET_INFO")
        {
            get_header_info();
        }
    }

    else
    {
        publish_stream("stdout", "Load a database to run this command.\n");
    }
}
/*
TODO:
[x] - loadExtension
[x] - key
[x] - rekey
[x] - isUnencrypted
[] - getHeaderInfo
[] - backup
*/

void interpreter::configure_impl()
{
}

nl::json interpreter::execute_request_impl(int execution_counter,
                                           const std::string& code,
                                           bool /*silent*/,
                                           bool /*store_history*/,
                                           nl::json /*user_expressions*/,
                                           bool /*allow_stdin*/)
{
    /*
        Executes either SQLite code or Jupyter Magic.
    */

    nl::json pub_data;
    std::string result = "";
    std::vector<std::string> tokenized_code = tokenizer(code);

    try
    {
        //Runs non-SQLite code
        if(is_magic(tokenized_code))
        {
            parse_code(tokenized_code);
        }
        //Runs SQLite code
        else
        {
            SQLite::Statement query(*m_db, code);
            std::stringstream query_result("");

            //Iterates through the columns and prints them
            while (query.executeStep())
            {
                for(int column = 0; column < query.getColumnCount(); column++) {
                    std::string name = query.getColumn(column);
                    query_result << name << std::endl;
                }
            }
            query.reset();
            result += query_result.str();
        }

        pub_data["text/plain"] = result;
        publish_execution_result(execution_counter, std::move(pub_data),
                                    nl::json::object());
        nl::json jresult;
        jresult["status"] = "ok";
        jresult["payload"] = nl::json::array();
        jresult["user_expressions"] = nl::json::object();
        return jresult;
    }

    catch (const std::runtime_error& err)
    {
        nl::json jresult;
        jresult["status"] = "error";
        jresult["ename"] = "Error";
        jresult["evalue"] = err.what();
        publish_stream("stderr", err.what());
        return jresult;
    }
}

nl::json interpreter::complete_request_impl(const std::string& /*code*/, int /*cursor_pos*/)
{
    nl::json jresult;
    jresult["status"] = "ok";
    return jresult;
};

nl::json interpreter::inspect_request_impl(const std::string& /*code*/,
                                           int /*cursor_pos*/,
                                           int /*detail_level*/)
{
    nl::json jresult;
    jresult["status"] = "ok";
    return jresult;
};

nl::json interpreter::is_complete_request_impl(const std::string& /*code*/)
{
    nl::json jresult;
    jresult["status"] = "complete";
    return jresult;
};

nl::json interpreter::kernel_info_request_impl()
{
    nl::json result;
    result["implementation"] = "xsqlite";
    result["implementation_version"] = "0.1.0";
    std::string banner = "";
    result["banner"] = banner;
    result["language_info"]["name"] = "sqlite";
    result["language_info"]["version"] = "";
    result["language_info"]["mimetype"] = "";
    result["language_info"]["file_extension"] = "";
    return result;
}

void interpreter::shutdown_request_impl()
{
}
}
