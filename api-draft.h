#include <string>
#include <vector>

namespace asyncfb
{

   class exception: public std::runtime_error
   {
      public:
         exception(const char *what): runtime_error(what) { }
   };

   struct database_info
   {
      int on_disc_structure, ods_minor, page_size, pages, buffers, sweep_interval;
      bool using_forced_writes, using_reserve;
   };

   struct database_statistics
   {
      int fetches, marks, reads, writes;
   };

   struct database_counts
   {
      int inserts, updates, deletes, index_reads, sequential_reads;
   };

   class database
   {
      public:
         database(
               const std::string &server,  const std::string &database,
               const std::string &user,    const std::string &password,
               const std::string &charset, const std::string &role,
               const std::string &params);
         ~database();
         void connect();
         void disconnect();
         bool connected();
         void create(int dialect);
         void drop();
         database_info info();
         database_statistics statistics();
         database_counts counts();
         std::vector<std::string> users();
         int dialect();
         const std::string &server();
         const std::string &database();
         const std::string &user();
         const std::string &password();
         const std::string &role();
         const std::string &charset();
         const std::string &params();
   };

   class transaction
   {
      public:
         enum access_mode { };
         enum isolation_level { };
         enum lock_resolution { };
         enum factory_flags { };
         enum table_reservation { };

         transaction(database &db, access_mode access, isolation_level isolation, lock_resolution lock, factory_flags flags);
         ~transaction();
         void start();
         bool started();
         void commit();
         void commit_retaining();
         void rollback();
         void add_reservation(database &db, const std::string &table, table_reservation reservation);
         void attach_database(database &db, access_mode access, isolation_level isolation, lock_resolution lock, factory_flags flags);
         void detach_database(database &db);
   };

   enum statement_type { };

   class variant
   {
   };

   class statement
   {
      public:
         enum field_type { };
         statement(database &db, transaction &tr, const std::string &sql = std::string());
         void prepare(const std::string &sql);
         void prepare_sync(const std::string &sql);
         void execute(const std::string &sql);
         void execute_sync(const std::string &sql);
         void execute_immediate(const std::string &sql);
         void execute_immediate_sync(const std::string &sql);
         void execute_cursor(const std::string &cursor, const std::string &sql = std::string());
         void execute_cursor_sync(const std::string &cursor, const std::string &sql = std::string());
         int affected_rows();
         statement_type type();
         const std::string sql();
         std::string plan();
         void close();

         void fetch(); // asynchronous
         bool fetch_sync();
         void fetch_all(std::vector<variant> &data); // asynchronous
         void fetch_all_sync(std::vector<variant> &data);
         void (*row_fetched)(statement &sender);

         bool is_null(int column);
         bool is_null(const std::string &column);
         bool get();
         int column_num(const std::string &column);
         const char *column_name();
         const char *column_alias();
         const char *column_table();
         field_type column_type();
         int column_subtype();
         int column_size();
         int column_scale();
         int columns();

         bool set();
         field_type parameter_type();
         int parameter_subtype();
         int parameter_size();
         int parameter_scale();
         int parameters();
   };

}
