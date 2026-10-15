#ifndef __PARSE_CSV_H__
#define __PARSE_CSV_H__


#define MAX_LINE_LENGTH 1024
#define MAX_VAR_NAME_LENGTH 100

typedef struct {
    char lineName[MAX_VAR_NAME_LENGTH];
    float* vars;
    int count;
} VariableLine;

#ifdef __cplusplus
extern "C" {
#endif

    VariableLine* parse_csv(const char *filename, int *line_count);
    VariableLine* parse_csv_only_data(const char *filename, int  skip_rows, int  skip_cols, int *line_count_out);
    void free_variable_lines(VariableLine* lines, int line_count);
    void print_variable_lines(const VariableLine* lines, int line_count);

#ifdef __cplusplus
}
#endif

#endif