/* -*- coding: utf-8; mode: C; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
  Copyright (C) 2009-2013  Kouhei Sutou <kou@clear-code.com>
  Copyright (C) 2014  Masafumi Yokoyama <myokoym@gmail.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License version 2.1 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "rb-grn.h"

#define SELF(object) ((RbGrnTableKeySupport *)DATA_PTR(object))

VALUE rb_mGrnTableKeySupport;

/*
 * Document-module: Groonga::Table::KeySupport
 *
 * 主キーを持つテーブルである {Groonga::Hash} と
 * {Groonga::PatriciaTrie} に主キーの機能を提供するモジュール。
 */

void
rb_grn_table_key_support_deconstruct (RbGrnTableKeySupport *rb_grn_table_key_support,
                                      grn_obj **table_key_support,
                                      grn_ctx **context,
                                      grn_obj **key,
                                      grn_id *domain_id,
                                      grn_obj **domain,
                                      grn_obj **value,
                                      grn_id *range_id,
                                      grn_obj **range,
                                      VALUE *columns)
{
    RbGrnTable *rb_grn_table;

    rb_grn_table = RB_GRN_TABLE(rb_grn_table_key_support);

    rb_grn_table_deconstruct(rb_grn_table, table_key_support, context,
                             domain_id, domain,
                             value, range_id, range,
                             columns);

    if (key)
        *key = rb_grn_table_key_support->key;
}

void
rb_grn_table_key_support_finalizer (grn_ctx *context,
                                    grn_obj *grn_object,
                                    RbGrnTableKeySupport *rb_grn_table_key_support)
{
    if (!context)
        return;

    if (rb_grn_table_key_support->key)
        grn_obj_unlink(context, rb_grn_table_key_support->key);
    rb_grn_table_key_support->key = NULL;

    rb_grn_table_finalizer(context, grn_object,
                           RB_GRN_TABLE(rb_grn_table_key_support));
}

void
rb_grn_table_key_support_bind (RbGrnTableKeySupport *rb_grn_table_key_support,
                               grn_ctx *context,
                               grn_obj *table_key_support)
{
    RbGrnObject *rb_grn_object;
    RbGrnTable *rb_grn_table;

    rb_grn_object = RB_GRN_OBJECT(rb_grn_table_key_support);

    rb_grn_table = RB_GRN_TABLE(rb_grn_table_key_support);
    rb_grn_table_bind(rb_grn_table, context, table_key_support);

    rb_grn_table_key_support->key =
        grn_obj_open(context, GRN_BULK, 0, rb_grn_object->domain_id);
}

static VALUE
rb_grn_table_key_support_inspect_content (VALUE self, VALUE inspected)
{
    RbGrnTableKeySupport *rb_grn_table;
    grn_ctx *context = NULL;
    grn_obj *table;

    rb_grn_table = SELF(self);
    if (!rb_grn_table)
        return inspected;

    rb_grn_table_key_support_deconstruct(SELF(self), &table, &context,
                                         NULL, NULL, NULL,
                                         NULL, NULL, NULL,
                                         NULL);
    if (!table)
        return inspected;
    if (!context)
        return inspected;

    {
        grn_obj value;
        grn_encoding encoding;

        rb_str_cat2(inspected, ", ");
        rb_str_cat2(inspected, "encoding: <");
        GRN_OBJ_INIT(&value, GRN_BULK, 0, GRN_ID_NIL);
        grn_obj_get_info(context, table, GRN_INFO_ENCODING, &value);
        encoding = *((grn_encoding *)GRN_BULK_HEAD(&value));
        grn_obj_unlink(context, &value);

        if (context->rc == GRN_SUCCESS) {
            rb_str_concat(inspected, rb_inspect(GRNENCODING2RVAL(encoding)));
        } else {
            rb_str_cat2(inspected, "invalid");
        }

        rb_str_cat2(inspected, ">");
    }

    {
        grn_obj *default_tokenizer;

        rb_str_cat2(inspected, ", ");
        rb_str_cat2(inspected, "default_tokenizer: ");
        default_tokenizer = grn_obj_get_info(context, table,
                                             GRN_INFO_DEFAULT_TOKENIZER,
                                             NULL);
        if (default_tokenizer) {
            rb_grn_object_inspect_object_content_name(inspected, context,
                                                      default_tokenizer);
        } else {
            rb_str_cat2(inspected, "(nil)");
        }
    }

    {
        grn_obj *normalizer;

        rb_str_cat2(inspected, ", ");
        rb_str_cat2(inspected, "normalizer: ");
        normalizer = grn_obj_get_info(context, table, GRN_INFO_NORMALIZER,
                                      NULL);
        if (normalizer) {
            rb_grn_object_inspect_object_content_name(inspected, context,
                                                      normalizer);
        } else {
            rb_str_cat2(inspected, "(nil)");
        }
    }

    {
        grn_obj token_filters;
        int n_token_filters;

        rb_str_cat2(inspected, ", ");
        rb_str_cat2(inspected, "token_filters: ");
        GRN_PTR_INIT(&token_filters, GRN_OBJ_VECTOR, GRN_ID_NIL);
        grn_obj_get_info(context, table, GRN_INFO_TOKEN_FILTERS,
                         &token_filters);
        n_token_filters = GRN_BULK_VSIZE(&token_filters) / sizeof(grn_obj *);
        if (n_token_filters > 0) {
            rb_str_cat2(inspected, "<");
            int i;
            for (i = 0; i < n_token_filters; i++) {
                grn_obj *token_filter = GRN_PTR_VALUE_AT(&token_filters, i);
                char name[GRN_TABLE_MAX_KEY_SIZE];
                int name_len;
                if (i > 0) {
                    rb_str_cat2(inspected, ",");
                }
                // TODO
                name_len = grn_obj_name(context, token_filter, name, GRN_TABLE_MAX_KEY_SIZE);
                rb_str_concat(inspected, rb_str_new(name, name_len));
            }
            rb_str_cat2(inspected, ">");
        } else {
            rb_str_cat2(inspected, "(nil)");
        }
        GRN_OBJ_FIN(context, &token_filters);
    }

    return inspected;
}

/*
 * Inspects the table.
 *
 * @overload inspect
 *   @return [String] the inspected string.
 */
static VALUE
rb_grn_table_key_support_inspect (VALUE self)
{
    VALUE inspected;

    inspected = rb_str_new2("");
    rb_grn_object_inspect_header(self, inspected);
    rb_grn_object_inspect_content(self, inspected);
    rb_grn_table_inspect_content(self, inspected);
    rb_grn_table_key_support_inspect_content(self, inspected);
    rb_grn_object_inspect_footer(self, inspected);

    return inspected;
}

static grn_id
rb_grn_table_key_support_add_raw (VALUE self, VALUE rb_key, int *added)
{
    grn_ctx *context;
    grn_obj *table;
    grn_id id, domain_id;
    grn_obj *key, *domain;

    rb_grn_table_key_support_deconstruct(SELF(self), &table, &context,
                                         &key, &domain_id, &domain,
                                         NULL, NULL, NULL,
                                         NULL);

    GRN_BULK_REWIND(key);
    RVAL2GRNKEY(rb_key, context, key, domain_id, domain, self);
    id = grn_table_add(context, table,
                       GRN_BULK_HEAD(key), GRN_BULK_VSIZE(key), added);
    rb_grn_context_check(context, self);

    return id;
}

/*
 * 主キーが _key_ のレコード追加し、追加したレコードを返す。レ
 * コードの追加に失敗した場合は +nil+ を返す。
 *
 * すでに同じキーのレコードが存在する場合は追加せずに同じレ
 * コードを返す。追加されたかどうかは
 * {Groonga::Record#added?} で調べることができる。 +true+ を返
 * したら追加されたということを示す。
 *
 * _values_ にはレコードのカラムに設定する値を指定する。省略
 * した場合または +nil+ を指定した場合はカラムは設定しない。カ
 * ラムの値は @{:カラム名1 => 値1, :カラム名2 => 値2,
 * ...}@ と指定する。
 *
 * @overload add(key, values=nil)
 *   @return [Groonga::Recordまたはnil]
 */
static VALUE
rb_grn_table_key_support_add (int argc, VALUE *argv, VALUE self)
{
    grn_id id;
    VALUE key, values;
    int added = GRN_FALSE;

    rb_scan_args(argc, argv, "11", &key, &values);
    id = rb_grn_table_key_support_add_raw(self, key, &added);
    if (GRN_ID_NIL == id) {
        return Qnil;
    } else {
        if (added) {
            return rb_grn_record_new_added(self, id, values);
        } else {
            return rb_grn_record_new(self, id, values);
        }
    }
}

grn_id
rb_grn_table_key_support_get (VALUE self, VALUE rb_key)
{
    grn_ctx *context;
    grn_obj *table, *key, *domain;
    grn_id id, domain_id;

    rb_grn_table_key_support_deconstruct(SELF(self), &table, &context,
                                         &key, &domain_id, &domain,
                                         NULL, NULL, NULL,
                                         NULL);

    GRN_BULK_REWIND(key);
    RVAL2GRNKEY(rb_key, context, key, domain_id, domain, self);
    id = grn_table_get(context, table,
                       GRN_BULK_HEAD(key), GRN_BULK_VSIZE(key));
    rb_grn_context_check(context, self);

    return id;
}

/*
 * _key_ を指定しない場合はテーブルのIDを返す。
 *
 * _key_ を指定した場合はテーブルの _key_ に対応するレコードの
 * IDを返す。
 *
 * @overload id
 *   @return [テーブルID]
 * @overload id(key)
 *   @return [レコードID]
 */
static VALUE
rb_grn_table_key_support_get_id (int argc, VALUE *argv, VALUE self)
{
    VALUE rb_key;

    rb_scan_args(argc, argv, "01", &rb_key);
    if (NIL_P(rb_key)) {
        return rb_grn_object_get_id(self);
    } else {
        grn_id id;

        id = rb_grn_table_key_support_get(self, rb_key);
        if (id == GRN_ID_NIL) {
            return Qnil;
        } else {
            return UINT2NUM(id);
        }
    }
}

/*
 * テーブルの _id_ に対応する主キーを返す。
 *
 * @overload key(id)
 *   @return [主キー]
 */
static VALUE
rb_grn_table_key_support_get_key (VALUE self, VALUE rb_id)
{
    grn_ctx *context;
    grn_obj *table, *key;
    grn_id id;
    int key_size;
    VALUE rb_key;

    rb_grn_table_key_support_deconstruct(SELF(self), &table, &context,
                                         &key, NULL, NULL,
                                         NULL, NULL, NULL,
                                         NULL);

    id = NUM2UINT(rb_id);
    GRN_BULK_REWIND(key);
    key_size = grn_table_get_key(context, table, id,
                                 GRN_BULK_HEAD(key), GRN_BULK_VSIZE(key));
    if (key_size == 0)
        return Qnil;

    if (GRN_BULK_VSIZE(key) < key_size) {
        grn_bulk_reserve(context, key, key_size);
        grn_table_get_key(context, table, id, GRN_BULK_HEAD(key), key_size);
    }

    rb_key = GRNKEY2RVAL(context, GRN_BULK_HEAD(key), key_size, table, self);
    return rb_key;
}

/*
 * テーブルに主キーが _key_ のレコードがあるならtrueを返す。
 *
 * @overload has_key?(key)
 */
static VALUE
rb_grn_table_key_support_has_key (VALUE self, VALUE rb_key)
{
    grn_ctx *context;
    grn_obj *table, *key, *domain;
    grn_id id, domain_id;

    rb_grn_table_key_support_deconstruct(SELF(self), &table, &context,
                                         &key, &domain_id, &domain,
                                         NULL, NULL, NULL,
                                         NULL);

    GRN_BULK_REWIND(key);
    RVAL2GRNKEY(rb_key, context, key, domain_id, domain, self);
    id = grn_table_get(context, table, GRN_BULK_HEAD(key), GRN_BULK_VSIZE(key));

    return id == GRN_ID_NIL ? Qfalse : Qtrue;
}

static VALUE
rb_grn_table_key_support_delete_by_key (VALUE self, VALUE rb_key)
{
    grn_ctx *context;
    grn_obj *table;
    grn_id domain_id;
    grn_obj *key, *domain;
    grn_rc rc;

    rb_grn_table_key_support_deconstruct(SELF(self), &table, &context,
                                         &key, &domain_id, &domain,
                                         NULL, NULL, NULL,
                                         NULL);

    GRN_BULK_REWIND(key);
    RVAL2GRNKEY(rb_key, context, key, domain_id, domain, self);
    rc = grn_table_delete(context, table,
                          GRN_BULK_HEAD(key), GRN_BULK_VSIZE(key));
    rb_grn_context_check(context, self);
    rb_grn_rc_check(rc, self);

    return Qnil;
}

/*
 * @overload delete(id, :id=>true)
 *   Delete a record that has ID @id@.
 *
 *   @param id [Integer] The ID of delete target record.
 *
 *   @return void
 *
 * @overload delete(key)
 *   Delete a record that has key @key@.
 *
 *   @param key [Object] The key of delete target record.
 *
 *   @return void
 *
 * @overload delete
 *   @yield [record]
 *     TODO: See #select.
 *   @yieldparam [Groonga::RecodExpressionBuilder] record
 *     TODO: See #select.
 *   @yieldreturn [Groonga::ExpressionBuilder]
 *     TODO: See #select.
 *
 *   @return void
 */
static VALUE
rb_grn_table_key_support_delete (int argc, VALUE *argv, VALUE self)
{
    VALUE rb_id_or_key, rb_options;
    VALUE rb_option_id;

    if (rb_block_given_p()) {
        return rb_grn_table_delete_by_expression(self);
    }

    rb_scan_args(argc, argv, "11", &rb_id_or_key, &rb_options);
    rb_grn_scan_options(rb_options,
                        "id", &rb_option_id,
                        NULL);
    if (RVAL2CBOOL(rb_option_id)) {
        return rb_grn_table_delete_by_id(self, rb_id_or_key);
    } else {
        return rb_grn_table_key_support_delete_by_key(self, rb_id_or_key);
    }
}

/*
 * _table_ の _key_ に対応する {Groonga::Record} を返す。
 *
 * 0.9.0から値ではなく {Groonga::Record} を返すようになった。
 *
 * @overload [](key)
 * @return [Groonga::Record]
 */
static VALUE
rb_grn_table_key_support_array_reference (VALUE self, VALUE rb_key)
{
    grn_id id;

    id = rb_grn_table_key_support_get(self, rb_key);
    if (id == GRN_ID_NIL) {
        return Qnil;
    } else {
        return rb_grn_record_new(self, id, Qnil);
    }
}

typedef struct _SetValueData
{
    VALUE self;
    grn_id id;
    grn_obj *table;
    RbGrnObject rb_grn_object;
} SetValueData;

static VALUE
set_value (VALUE args, SetValueData *data)
{
    VALUE rb_name, rb_value, rb_column;
    RbGrnObject *rb_grn_object;

    rb_name = rb_ary_entry(args, 0);
    rb_value = rb_ary_entry(args, 1);

    rb_column = rb_grn_table_get_column(data->self, rb_name);
    if (NIL_P(rb_column)) {
        rb_raise(rb_eGrnNoSuchColumn,
                 "no such column: <%s>: <%s>",
                 rb_grn_inspect(rb_name), rb_grn_inspect(data->self));
    }

    rb_grn_object = RB_GRN_OBJECT(DATA_PTR(rb_column));
    return rb_grn_object_set_raw(rb_grn_object,
                                 data->id, rb_value, GRN_OBJ_SET, data->self);
}

/*
 * _table_ の _key_ に対応するカラム _column_name_ の値を設定する。
 * _key_ に対応するレコードがない場合は新しく作成される。
 *
 * 0.9.0から値ではなくカラムの値を設定するようになった。
 *
 * @overload []=(key, values)
 *   @param [::Hash] values
 *     keyに対応させるカラムの値。{ :column_name => value, ... }の形で設定する。
 */
static VALUE
rb_grn_table_key_support_array_set (VALUE self, VALUE rb_key, VALUE rb_values)
{
    grn_id id;
    SetValueData data;
    grn_ctx *context;
    grn_obj *table;

    rb_grn_table_key_support_deconstruct(SELF(self), &table, &context,
                                         NULL, NULL, NULL,
                                         NULL, NULL, NULL,
                                         NULL);

    id = rb_grn_table_key_support_add_raw(self, rb_key, NULL);

    if (id == GRN_ID_NIL) {
        rb_raise(rb_eGrnError,
                 "failed to add record: %s",
                 rb_grn_inspect(rb_ary_new3(3, self, rb_key, rb_values)));
    }

    data.self = self;
    data.id = id;
    data.table = table;
    data.rb_grn_object.context = context;
    rb_iterate(rb_each, rb_values, set_value, (VALUE)&data);

    return Qnil;
}

/*
 * _table_ の _key_ に対応するカラム _name_ の値を設定する。
 * _key_ に対応するレコードがない場合は新しく作成される。
 *
 * @overload set_column_value(key, name, value)
 * @overload set_column_value(id, name, value, {:id=>true})
 */
static VALUE
rb_grn_table_key_support_set_column_value (int argc, VALUE *argv, VALUE self)
{
    grn_id id;
    VALUE rb_key, rb_id_or_key, rb_name, rb_value, rb_options;

    rb_scan_args(argc, argv, "31",
                 &rb_id_or_key, &rb_name, &rb_value, &rb_options);
    if (!NIL_P(rb_options)) {
        VALUE rb_option_id;
        rb_grn_scan_options(rb_options,
                            "id", &rb_option_id,
                            NULL);
        if (RVAL2CBOOL(rb_option_id)) {
            VALUE rb_id = rb_id_or_key;
            return rb_grn_table_set_column_value(self, rb_id, rb_name, rb_value);
        }
    }

    rb_key = rb_id_or_key;
    id = rb_grn_table_key_support_add_raw(self, rb_key, NULL);
    if (id == GRN_ID_NIL) {
        rb_raise(rb_eGrnError,
                 "failed to add record: %s",
                 rb_grn_inspect(rb_ary_new3(4,
                                            self, rb_key,
                                            rb_name, rb_value)));
    }

    return rb_grn_table_set_column_value_raw(self, id, rb_name, rb_value);
}

/*
 * _table_ の _key_ に対応するカラム _name_ の値を設定する。
 *
 * TODO: _key_ に対応するレコードがない場合は例外？
 *
 * @overload column_value(key, name)
 * @overload column_value(id, name, :id=>true)
 */
static VALUE
rb_grn_table_key_support_get_column_value (int argc, VALUE *argv, VALUE self)
{
    grn_id id;
    VALUE rb_key, rb_id_or_key, rb_name, rb_options;

    rb_scan_args(argc, argv, "21", &rb_id_or_key, &rb_name, &rb_options);
    if (!NIL_P(rb_options)) {
        VALUE rb_option_id;
        rb_grn_scan_options(rb_options,
                            "id", &rb_option_id,
                            NULL);
        if (RVAL2CBOOL(rb_option_id)) {
            VALUE rb_id = rb_id_or_key;
            return rb_grn_table_get_column_value(self, rb_id, rb_name);
        }
    }

    rb_key = rb_id_or_key;
    id = rb_grn_table_key_support_get(self, rb_key);
    if (id == GRN_ID_NIL) {
        return Qnil;
    }

    return rb_grn_table_get_column_value_raw(self, id, rb_name);
}

static VALUE
rb_grn_table_key_support_get_value_by_key (VALUE self, VALUE rb_key)
{
    grn_ctx *context;
    grn_obj *table, *value, *range;
    grn_id id;

    id = rb_grn_table_key_support_get(self, rb_key);

    if (id == GRN_ID_NIL)
        return Qnil;

    rb_grn_table_key_support_deconstruct(SELF(self), &table, &context,
                                         NULL, NULL, NULL,
                                         &value, NULL, &range,
                                         NULL);
    GRN_BULK_REWIND(value);
    grn_obj_get_value(context, table, id, value);
    rb_grn_context_check(context, self);

    return GRNBULK2RVAL(context, value, range, self);
}

/*
 * _table_ の _id_ または _key_ に対応する値を返す。
 *
 * @overload value(id, :id=>true)
 * @overload value(key)
 * @return _id_ もしくは _key_ に対応する値
 */
static VALUE
rb_grn_table_key_support_get_value (int argc, VALUE *argv, VALUE self)
{
    VALUE rb_id_or_key, rb_options;
    grn_bool use_key;

    rb_scan_args(argc, argv, "11", &rb_id_or_key, &rb_options);

    if (NIL_P(rb_options)) {
        use_key = GRN_TRUE;
    } else {
        VALUE rb_option_id;

        rb_grn_scan_options(rb_options,
                            "id", &rb_option_id,
                            NULL);
        use_key = !RVAL2CBOOL(rb_option_id);
    }

    if (use_key) {
        return rb_grn_table_key_support_get_value_by_key(self, rb_id_or_key);
    } else {
        return rb_grn_table_get_value(self, rb_id_or_key);
    }
}

static VALUE
rb_grn_table_key_support_set_value_by_key (VALUE self,
                                           VALUE rb_key, VALUE rb_value)
{
    grn_ctx *context;
    grn_obj *table;
    grn_id id;
    grn_obj *value;
    grn_rc rc;

    if (NIL_P(rb_key)) {
        rb_raise(rb_eArgError, "key should not be nil: <%s>",
                 rb_grn_inspect(self));
    }

    rb_grn_table_key_support_deconstruct(SELF(self), &table, &context,
                                         NULL, NULL, NULL,
                                         &value, NULL, NULL,
                                         NULL);

    id = rb_grn_table_key_support_add_raw(self, rb_key, NULL);
    if (GRN_ID_NIL == id) {
        rb_raise(rb_eGrnError,
                 "failed to add new record with key: <%s>: <%s>",
                 rb_grn_inspect(rb_key),
                 rb_grn_inspect(self));
    }

    GRN_BULK_REWIND(value);
    RVAL2GRNBULK(rb_value, context, value);
    rc = grn_obj_set_value(context, table, id, value, GRN_OBJ_SET);
    rb_grn_context_check(context, self);
    rb_grn_rc_check(rc, self);

    return rb_value;
}

/*
 * _table_ の _id_ または _key_ に対応する値を _value_ に設定する。
 * 既存の値は上書きされる。
 *
 * @overload set_value(id, value, :id=>true)
 * @overload set_value(key, value)
 */
static VALUE
rb_grn_table_key_support_set_value (int argc, VALUE *argv, VALUE self)
{
    VALUE rb_id_or_key, rb_value, rb_options;
    grn_bool use_key;

    rb_scan_args(argc, argv, "21", &rb_id_or_key, &rb_value, &rb_options);

    if (NIL_P(rb_options)) {
        use_key = GRN_TRUE;
    } else {
        VALUE rb_option_id;

        rb_grn_scan_options(rb_options,
                            "id", &rb_option_id,
                            NULL);
        use_key = !RVAL2CBOOL(rb_option_id);
    }

    if (use_key) {
        return rb_grn_table_key_support_set_value_by_key(self,
                                                         rb_id_or_key,
                                                         rb_value);
    } else {
        return rb_grn_table_set_value(self, rb_id_or_key, rb_value);
    }
}

/*
 * {Groonga::IndexColumn} で使用するトークナイザを返す。
 *
 * @overload default_tokenizer
 *   @return [nilまたはGroonga::Procedure]
 */
static VALUE
rb_grn_table_key_support_get_default_tokenizer (VALUE self)
{
    grn_ctx *context = NULL;
    grn_obj *table;
    grn_obj *tokenizer = NULL;

    rb_grn_table_key_support_deconstruct(SELF(self), &table, &context,
                                         NULL, NULL, NULL,
                                         NULL, NULL, NULL,
                                         NULL);

    tokenizer = grn_obj_get_info(context, table, GRN_INFO_DEFAULT_TOKENIZER,
                                 NULL);
    rb_grn_context_check(context, self);

    return GRNOBJECT2RVAL(Qnil, context, tokenizer, GRN_FALSE);
}

/*
 * {Groonga::IndexColumn} で使用するトークナイザを設定する。
 *
 * @example
 *   # 2-gramを使用。
 *   table.default_tokenizer = "TokenBigram"
 *   # オブジェクトで指定
 *   table.default_tokenizer = Groonga::Context.default["TokenBigram"]
 *   # オブジェクトIDで指定
 *   table.default_tokenizer = Groonga::Type::BIGRAM
 *   # N-gram用のトークナイザを使うときはGroonga::IndexColumn
 *   # には自動的に:with_section => trueが指定される。
 *   index = table.define_index_column("blog_content", "Blogs",
 *                                     :source => "content")
 *   p index # -> #<Groonga::IndexColumn ... flags: <WITH_POSITION|...>>
 *
 *   # MeCabを使用
 *   table.default_tokenizer = "TokenMecab"
 *
 * @overload default_tokenizer=(tokenizer)
 */
static VALUE
rb_grn_table_key_support_set_default_tokenizer (VALUE self, VALUE rb_tokenizer)
{
    grn_ctx *context;
    grn_obj *table;
    grn_obj *tokenizer;
    grn_rc rc;

    rb_grn_table_key_support_deconstruct(SELF(self), &table, &context,
                                         NULL, NULL, NULL,
                                         NULL, NULL, NULL,
                                         NULL);

    tokenizer = RVAL2GRNOBJECT(rb_tokenizer, &context);
    rc = grn_obj_set_info(context, table,
                          GRN_INFO_DEFAULT_TOKENIZER, tokenizer);
    rb_grn_context_check(context, self);
    rb_grn_rc_check(rc, self);

    return Qnil;
}

/*
 * Returns the normalizer that is used by {Groonga::IndexColumn}.
 *
 * @overload normalizer
 *   @return [nil, Groonga::Procedure]
 */
static VALUE
rb_grn_table_key_support_get_normalizer (VALUE self)
{
    grn_ctx *context = NULL;
    grn_obj *table;
    grn_obj *normalizer = NULL;

    rb_grn_table_key_support_deconstruct(SELF(self), &table, &context,
                                         NULL, NULL, NULL,
                                         NULL, NULL, NULL,
                                         NULL);

    normalizer = grn_obj_get_info(context, table, GRN_INFO_NORMALIZER, NULL);
    rb_grn_context_check(context, self);

    return GRNOBJECT2RVAL(Qnil, context, normalizer, GRN_FALSE);
}

/*
 * Specifies the normalizer used by {Groonga::IndexColumn}.
 *
 * @example
 *   # Uses NFKC normalizer.
 *   table.normalizer = "NormalizerNFKC51"
 *   # Specifies normalizer object.
 *   table.normalizer = Groonga::Context["NormalizerNFKC51"]
 *   # Uses auto normalizer that is a normalizer for backward compatibility.
 *   table.normalizer = "TNormalizerAuto"
 *
 * @overload normalizer=(name)
 *    @param [String] name Set a nomalizer named @name@.
 *
 * @overload normalizer=(normalizer)
 *    @param [Groonga::Procedure] normalizer Set the normalizer object.
 *
 * @overload normalizer=(normalizer)
 *    @param [nil] normalizer Unset normalizer.
 */
static VALUE
rb_grn_table_key_support_set_normalizer (VALUE self, VALUE rb_normalizer)
{
    grn_ctx *context;
    grn_obj *table;
    grn_obj *normalizer;
    grn_rc rc;

    rb_grn_table_key_support_deconstruct(SELF(self), &table, &context,
                                         NULL, NULL, NULL,
                                         NULL, NULL, NULL,
                                         NULL);

    normalizer = RVAL2GRNOBJECT(rb_normalizer, &context);
    rc = grn_obj_set_info(context, table, GRN_INFO_NORMALIZER, normalizer);
    rb_grn_context_check(context, self);
    rb_grn_rc_check(rc, self);

    return Qnil;
}

/*
 * TODO
 *
 * @overload token_filters
 *   @return [nilまたはGroonga::Procedure]
 */
static VALUE
rb_grn_table_key_support_get_token_filters (VALUE self)
{
    grn_ctx *context = NULL;
    grn_obj *table;
    grn_obj *token_filters = NULL;

    rb_grn_table_key_support_deconstruct(SELF(self), &table, &context,
                                         NULL, NULL, NULL,
                                         NULL, NULL, NULL,
                                         NULL);

    token_filters = grn_obj_get_info(context, table, GRN_INFO_TOKEN_FILTERS,
                                 NULL);
    rb_grn_context_check(context, self);

    return GRNOBJECT2RVAL(Qnil, context, token_filters, GRN_FALSE);
}

/*
 * TODO
 */
static VALUE
rb_grn_table_key_support_set_token_filters (VALUE self, VALUE rb_token_filters)
{
    grn_ctx *context;
    grn_obj *table;
    grn_obj token_filters;
    grn_rc rc;
    VALUE rb_token_filter_vector = rb_ary_new();
    const char *name = NULL;
    unsigned name_size = 0;

    rb_grn_table_key_support_deconstruct(SELF(self), &table, &context,
                                         NULL, NULL, NULL,
                                         NULL, NULL, NULL,
                                         NULL);

    // TODO
    name = StringValuePtr(rb_token_filters);
    name_size = RSTRING_LEN(rb_token_filters);

    GRN_PTR_INIT(&token_filters, GRN_OBJ_VECTOR, 0);
    GRN_PTR_PUT(context,
                &token_filters,
                grn_ctx_get(context,
                            name,
                            name_size));
    rc = grn_obj_set_info(context, table, GRN_INFO_TOKEN_FILTERS, &token_filters);
    rb_grn_context_check(context, self);
    rb_grn_rc_check(rc, self);
    grn_obj_unlink(context, &token_filters);

    return Qnil;
}

/*
 * キーを正規化する場合は +true+ 、正規化しない場合は +false+ を返
 * す。
 *
 * @overload normalize_key?
 */
static VALUE
rb_grn_table_key_support_normalize_key_p (VALUE self)
{
    VALUE normalizer;

    normalizer = rb_grn_table_key_support_get_normalizer(self);
    return CBOOL2RVAL(!NIL_P(normalizer));
}

/*
 * テーブルでキーを使えるなら +true+ 、使えないなら +false+ を返
 * す。キーを使えないテーブルは {Groonga::Array} だけ。
 *
 * @overload support_key?
 */
static VALUE
rb_grn_table_key_support_support_key_p (VALUE self)
{
    grn_obj *table;

    rb_grn_table_key_support_deconstruct(SELF(self), &table, NULL,
                                         NULL, NULL, NULL,
                                         NULL, NULL, NULL,
                                         NULL);
    return table->header.domain == GRN_ID_NIL ? Qfalse : Qtrue;
}

/*
 * Tokenize a string using the table as lexicon.
 *
 * @overload tokenize(string, options={})
 *   @param [String] string The string to be tokenized.
 *   @param [::Hash] options
 *   @option options [Bool] :add (true) Adds a new token to the table if true.
 *      Returned tokens include the new token. Otherwise, a new token is
 *      just ignored.
 *   @return [::Array<Groonga::Record>] Tokenized tokens.
 */
static VALUE
rb_grn_table_key_support_tokenize (int argc, VALUE *argv, VALUE self)
{
    VALUE rb_string, rb_add_p;
    VALUE rb_options;
    VALUE rb_tokens = Qnil;
    grn_ctx *context;
    grn_obj *table;
    char *string;
    int string_size;
    grn_bool add_p;
    grn_obj tokens;

    rb_scan_args(argc, argv, "11", &rb_string, &rb_options);
    rb_grn_scan_options(rb_options,
                        "add", &rb_add_p,
                        NULL);
    if (NIL_P(rb_add_p)) {
        rb_add_p = Qtrue;
    }

    rb_grn_table_key_support_deconstruct(SELF(self), &table, &context,
                                         NULL, NULL, NULL,
                                         NULL, NULL, NULL,
                                         NULL);

    string = StringValueCStr(rb_string);
    string_size = RSTRING_LEN(rb_string);

    add_p = RVAL2CBOOL(rb_add_p);

    GRN_RECORD_INIT(&tokens, GRN_OBJ_VECTOR, grn_obj_id(context, table));
    grn_table_tokenize(context, table, string, string_size, &tokens, add_p);
    if (context->rc == GRN_SUCCESS) {
        rb_tokens = GRNUVECTOR2RVAL(context, &tokens, table, self);
    }
    grn_obj_unlink(context, &tokens);
    rb_grn_context_check(context, self);

    return rb_tokens;
}

void
rb_grn_init_table_key_support (VALUE mGrn)
{
    rb_mGrnTableKeySupport = rb_define_module_under(rb_cGrnTable, "KeySupport");
    rb_include_module(rb_mGrnTableKeySupport, rb_mGrnEncodingSupport);

    rb_define_method(rb_mGrnTableKeySupport, "inspect",
                     rb_grn_table_key_support_inspect, 0);

    rb_define_method(rb_mGrnTableKeySupport, "add",
                     rb_grn_table_key_support_add, -1);
    rb_define_method(rb_mGrnTableKeySupport, "id",
                     rb_grn_table_key_support_get_id, -1);
    rb_define_method(rb_mGrnTableKeySupport, "key",
                     rb_grn_table_key_support_get_key, 1);
    rb_define_method(rb_mGrnTableKeySupport, "has_key?",
                     rb_grn_table_key_support_has_key, 1);

    rb_define_method(rb_mGrnTableKeySupport, "delete",
                     rb_grn_table_key_support_delete, -1);

    rb_define_method(rb_mGrnTableKeySupport, "[]",
                     rb_grn_table_key_support_array_reference, 1);
    rb_define_method(rb_mGrnTableKeySupport, "[]=",
                     rb_grn_table_key_support_array_set, 2);

    rb_define_method(rb_mGrnTableKeySupport, "column_value",
                     rb_grn_table_key_support_get_column_value, -1);
    rb_define_method(rb_mGrnTableKeySupport, "set_column_value",
                     rb_grn_table_key_support_set_column_value, -1);

    rb_define_method(rb_mGrnTableKeySupport, "value",
                     rb_grn_table_key_support_get_value, -1);
    rb_define_method(rb_mGrnTableKeySupport, "set_value",
                     rb_grn_table_key_support_set_value, -1);

    rb_define_method(rb_mGrnTableKeySupport, "default_tokenizer",
                     rb_grn_table_key_support_get_default_tokenizer, 0);
    rb_define_method(rb_mGrnTableKeySupport, "default_tokenizer=",
                     rb_grn_table_key_support_set_default_tokenizer, 1);

    rb_define_method(rb_mGrnTableKeySupport, "normalizer",
                     rb_grn_table_key_support_get_normalizer, 0);
    rb_define_method(rb_mGrnTableKeySupport, "normalizer=",
                     rb_grn_table_key_support_set_normalizer, 1);

    rb_define_method(rb_mGrnTableKeySupport, "token_filters",
                     rb_grn_table_key_support_get_token_filters, 0);
    rb_define_method(rb_mGrnTableKeySupport, "token_filters=",
                     rb_grn_table_key_support_set_token_filters, 1);

    rb_define_method(rb_mGrnTableKeySupport, "normalize_key?",
                     rb_grn_table_key_support_normalize_key_p, 0);

    rb_define_method(rb_mGrnTableKeySupport, "support_key?",
                     rb_grn_table_key_support_support_key_p, 0);

    rb_define_method(rb_mGrnTableKeySupport, "tokenize",
                     rb_grn_table_key_support_tokenize, -1);
}
