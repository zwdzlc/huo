#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "base_util.h"
#include "core_functions.h"
#include "process_defs.h"
#include "execute.h"

struct Map * make_args_map(struct Tree * ast, struct Tree_map * defined, struct Map * let_map, int idx, int max_depth){
    struct Map * arguments = malloc(sizeof(struct Map));
    arguments->size = 0;
    if (defined->trees[idx]->size <= ast->size) {
        ERROR("Not enough arguments!: %i < %i", defined->trees[idx]->size - 1, ast->size);
    }
    for(int i = 0; i < ast->size; i++){
        char t = defined->trees[idx]->children[i+1]->content.type;
        if (t != KEYWORD) {
            ERROR("Invalid type for argument: '%c' != KEYWORD", t);
        }
        struct Keyval * store = malloc(sizeof(struct Keyval));
        struct Value val = execute(ast->children[i], defined, let_map, max_depth - 1);
        arguments->members[i] = store;
        arguments->members[i]->key = copy_value_heap(&defined->trees[idx]->children[i+1]->content);
        arguments->members[i]->val = copy_value_heap(&val);
        arguments->size++;
    }
    return arguments;
}

struct Tree * populate_args(struct Map * arguments, struct Tree * ast){
    if(ast->type == 'k' && !ast->size){
        for(int i = 0; i < arguments->size; i++){
            if (ast->content.type != STRING && ast->content.type != KEYWORD) {
                //ERROR("Variable already bound?");
            } else if(string_matches(&arguments->members[i]->key->data.str, &ast->content.data.str)){
                ast->content = copy_value_stack(arguments->members[i]->val);
                if(arguments->members[i]->val->type == STRING){
                    ast->type = 's';
                }
                else if(arguments->members[i]->val->type == KEYWORD) {
                    ast->type = 'k';
                }
                else if(arguments->members[i]->val->type == ARRAY) {
                    ast->type = 'a';
                } else {
                    ast->type = 'n';
                }
                return ast;
            }
        }
    }
    if(ast->type == 'a'){
        populate_array(arguments, ast->content.data.array);
    }
    else {
        for(int i = 0; i < ast->size; i++){
            populate_args(arguments, ast->children[i]);
        }
    }
    return ast;
}

void populate_array(struct Map * arguments, struct Value_array * array){
    for(int i = 0; i < array->size; i++){
        if(array->values[i]->type == KEYWORD){
            for(int l = 0; l < arguments->size; l++){
                if(string_matches(&array->values[i]->data.str, &arguments->members[l]->key->data.str)){
                        array->values[i] = copy_value_heap(arguments->members[l]->val);
                        if (array->values[i]->type != KEYWORD) {
                            break;
                        }
                }
            }
        } else if(array->values[i]->type == 'a'){
            populate_array(arguments, array->values[i]->data.array);
        }
    }
}

struct Tree * get_defined_body(struct Tree * function){
    // just pulls the function body out of a (def...)
    int check = 1;
    int index = 1;
    while(check){
        if (function->size <= index) {
            ERROR("No function body!");
        }
        if(!function->children[index]->size){
            index++;
        } else {
            check = 0;
            function = function->children[index];
        }
    }
    return function;
}

int is_defined_func(struct Tree_map * defined, struct String key){
    assert(string_is_sane(&key));
    for(int i = 0; i < defined->size; i++){
        if(string_matches(defined->names[i], &key)){
            return i;
        }
    }
    return -1;
}
