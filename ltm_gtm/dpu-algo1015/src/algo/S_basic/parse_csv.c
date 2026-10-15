#include "sw_base.hpp"
#include <ctype.h>

//No skip lines, Automatically takes the first column as "linename"
VariableLine* parse_csv(const char *filename, int *line_count) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Could not open file");
        exit(EXIT_FAILURE);
    }

    VariableLine* lines = NULL;
    char line[MAX_LINE_LENGTH];
    int total_lines = 0;

    while (fgets(line, sizeof(line), file)) {
        VariableLine variable_line; 
        variable_line.vars = NULL;
        variable_line.count = 0;
        memset(variable_line.lineName, 0, sizeof(variable_line.lineName));

        char* token = strtok(line, ",");

        // 获取当前行的名字
        if (token) {
            strncpy(variable_line.lineName, token, MAX_VAR_NAME_LENGTH - 1);
            variable_line.lineName[MAX_VAR_NAME_LENGTH - 1] = '\0'; // 确保以 null 终止

            // 获取剩余的变量
            token = strtok(NULL, ",");
        }

        while (token) {
            variable_line.vars = realloc(variable_line.vars, (variable_line.count + 1) * sizeof(float));
            if (!variable_line.vars) {
                perror("Memory allocation failed");
                exit(EXIT_FAILURE);
            }

            // 获取变量值
            variable_line.vars[variable_line.count] = atof(token);
            variable_line.count++;
            token = strtok(NULL, ",");
        }

        lines = realloc(lines, (total_lines + 1) * sizeof(VariableLine));
        if (!lines) {
            perror("Memory allocation failed");
            exit(EXIT_FAILURE);
        }
        lines[total_lines] = variable_line;
        total_lines++;
    }

    fclose(file);
    *line_count = total_lines;
    return lines;
}

VariableLine* parse_csv_only_data(const char *filename,int  skip_rows, int skip_cols, int *line_count_out)
{
    FILE *file = fopen(filename, "r");
    if (!file) { perror("fopen"); exit(EXIT_FAILURE); }

    VariableLine *lines = NULL;
    int total_lines = 0;
    char line[MAX_LINE_LENGTH];
    int line_no = 0;            

    while (fgets(line, sizeof line, file)) {

        if (line_no < skip_rows) { line_no++; continue; }

        VariableLine cur = { .vars = NULL,.count = 0 };

        int col_idx = 0;
        for (char *tok = strtok(line, ","); tok; tok = strtok(NULL, ","), col_idx++) {

            if (col_idx < skip_cols)     
                continue;


            while (*tok && isspace((unsigned char)*tok)) ++tok;
            size_t len = strlen(tok);
            while (len && isspace((unsigned char)tok[len - 1])) tok[--len] = '\0';
            if (len == 0)
                continue;

            cur.vars = realloc(cur.vars, (cur.count + 1) * sizeof(float));
            ASSERT(NULL != cur.vars);
            cur.vars[cur.count++] = atof(tok);
        }

        lines = realloc(lines, (total_lines + 1) * sizeof(VariableLine));
        ASSERT(NULL != lines);
        lines[total_lines++] = cur;
    }

    fclose(file);
    *line_count_out = total_lines;
    return lines;
}


void free_variable_lines(VariableLine* lines, int line_count) {
    for (int i = 0; i < line_count; i++) {
        free(lines[i].vars);
    }
    free(lines);
}

void print_variable_lines(const VariableLine* lines, int line_count) {
    for (int i = 0; i < line_count; i++) {
        printf("Line name: %s\n", lines[i].lineName);
        for (int j = 0; j < lines[i].count; j++) {
            printf("\tValue: %f\n", lines[i].vars[j]);
        }
    }
}
//
//int main() {
//    int line_count = 0;
//
//    // 解析 CSV 文件
//    VariableLine* lines = parse_csv("SW/Y2R_v0/Y2R_coefficients_10bits.csv", &line_count);
//
//    // 输出解析结果
//    print_variable_lines(lines, line_count);
//
//    // 释放内存
//    free_variable_lines(lines, line_count);
//
//    return 0;
//}
