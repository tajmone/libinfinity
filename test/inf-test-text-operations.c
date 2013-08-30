/* libinfinity - a GObject-based infinote implementation
 * Copyright (C) 2007-2013 Armin Burgmeier <armin@arbur.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#include <libinftext/inf-text-default-insert-operation.h>
#include <libinftext/inf-text-default-delete-operation.h>
#include <libinftext/inf-text-insert-operation.h>
#include <libinftext/inf-text-delete-operation.h>
#include <libinftext/inf-text-default-buffer.h>
#include <libinftext/inf-text-buffer.h>
#include <libinftext/inf-text-chunk.h>
#include <libinftext/inf-text-user.h>
#include <libinfinity/adopted/inf-adopted-no-operation.h>
#include <libinfinity/adopted/inf-adopted-operation.h>
#include <libinfinity/adopted/inf-adopted-user.h>

#include <string.h>

typedef struct {
  guint total;
  guint passed;
} test_result;

typedef enum {
  OP_INS,
  OP_DEL,
  OP_SPLIT
} operation_type;

typedef struct _operation_def operation_def;
struct _operation_def {
  operation_type type;
  guint offset;
  const gchar* text;
  const operation_def* split_1;
  const operation_def* split_2;
};

static const operation_def SPLIT_OPS[] = {
  { OP_DEL, 0, GUINT_TO_POINTER(1) },
  { OP_DEL, 1, GUINT_TO_POINTER(1) },
  { OP_DEL, 2, GUINT_TO_POINTER(1) },
  { OP_INS, 0, "a" },
  { OP_INS, 1, "b" }
};

static const operation_def OPERATIONS[] = {
  { OP_INS, 4, "a" },
  { OP_INS, 4, "b" },
  { OP_INS, 4, "c" },
  { OP_INS, 4, "a" },
  { OP_INS, 2, "ac" },
  { OP_INS, 3, "bc" },
  { OP_INS, 2, "gro" },
  { OP_DEL, 0, GUINT_TO_POINTER(1) },
  { OP_DEL, 0, GUINT_TO_POINTER(5) },
  { OP_DEL, 2, GUINT_TO_POINTER(7) },
  { OP_DEL, 1, GUINT_TO_POINTER(9) },
  /* del vs. del */
  { OP_SPLIT, 0, NULL, &SPLIT_OPS[0], &SPLIT_OPS[2] },
  { OP_SPLIT, 0, NULL, &SPLIT_OPS[2], &SPLIT_OPS[0] },
  { OP_SPLIT, 0, NULL, &SPLIT_OPS[0], &SPLIT_OPS[1] },
  { OP_SPLIT, 0, NULL, &SPLIT_OPS[1], &SPLIT_OPS[0] },
  /* del vs. ins */
  { OP_SPLIT, 0, NULL, &SPLIT_OPS[1], &SPLIT_OPS[3] },
  { OP_SPLIT, 0, NULL, &SPLIT_OPS[1], &SPLIT_OPS[4] },
  { OP_SPLIT, 0, NULL, &SPLIT_OPS[3], &SPLIT_OPS[1] },
  { OP_SPLIT, 0, NULL, &SPLIT_OPS[4], &SPLIT_OPS[1] },
};

static const gchar EXAMPLE_DOCUMENT[] = "abcdefghijklmnopqrstuvwxyz";

static InfAdoptedOperation*
def_to_operation(const operation_def* def,
                 InfTextChunk* document,
                 InfAdoptedUser* user)
{
  InfTextChunk* chunk;
  InfAdoptedOperation* operation;
  InfAdoptedOperation* operation1;
  InfAdoptedOperation* operation2;
  InfTextDefaultBuffer* buf;
  InfTextChunk* new_document;

  switch(def->type)
  {
  case OP_INS:
    chunk = inf_text_chunk_new("UTF-8");
    inf_text_chunk_insert_text(
      chunk,
      0,
      def->text,
      strlen(def->text),
      strlen(def->text),
      inf_user_get_id(INF_USER(user))
    );

    operation = INF_ADOPTED_OPERATION(
      inf_text_default_insert_operation_new(def->offset, chunk)
    );

    inf_text_chunk_free(chunk);
    break;
  case OP_DEL:
    chunk = inf_text_chunk_substring(
      document,
      def->offset,
      GPOINTER_TO_UINT(def->text)
    );

    operation = INF_ADOPTED_OPERATION(
      inf_text_default_delete_operation_new(def->offset, chunk)
    );

    inf_text_chunk_free(chunk);
    break;
  case OP_SPLIT:
    operation1 = def_to_operation(def->split_1, document, user);

    buf = inf_text_default_buffer_new("UTF-8");
    inf_text_buffer_insert_chunk(INF_TEXT_BUFFER(buf), 0, document, NULL);
    inf_adopted_operation_apply(operation1, user, INF_BUFFER(buf));

    new_document = inf_text_buffer_get_slice(
      INF_TEXT_BUFFER(buf),
      0,
      inf_text_buffer_get_length(INF_TEXT_BUFFER(buf))
    );

    g_object_unref(buf);

    operation2 = def_to_operation(def->split_2, new_document, user);
    inf_text_chunk_free(new_document);

    operation = INF_ADOPTED_OPERATION(
      inf_adopted_split_operation_new(operation1, operation2)
    );

    g_object_unref(operation1);
    g_object_unref(operation2);
    break;
  default:
    g_assert_not_reached();
    break;
  }

  return operation;
}

static gboolean
test_undo(InfAdoptedOperation* op,
          InfAdoptedUser* user)
{
  InfTextDefaultBuffer* first;
  InfTextDefaultBuffer* second;
  InfAdoptedOperation* reverted;
  InfTextChunk* first_chunk;
  InfTextChunk* second_chunk;
  int result;

  first = inf_text_default_buffer_new("UTF-8");

  inf_text_buffer_insert_text(
    INF_TEXT_BUFFER(first),
    0,
    EXAMPLE_DOCUMENT,
    strlen(EXAMPLE_DOCUMENT),
    strlen(EXAMPLE_DOCUMENT),
    NULL
  );

  second = inf_text_default_buffer_new("UTF-8");
  inf_text_buffer_insert_text(
    INF_TEXT_BUFFER(second),
    0,
    EXAMPLE_DOCUMENT,
    strlen(EXAMPLE_DOCUMENT),
    strlen(EXAMPLE_DOCUMENT),
    NULL
  );

  inf_adopted_operation_apply(op, user, INF_BUFFER(first));
  reverted = inf_adopted_operation_revert(op);
  inf_adopted_operation_apply(reverted, user, INF_BUFFER(first));
  g_object_unref(reverted);

  first_chunk = inf_text_buffer_get_slice(
    INF_TEXT_BUFFER(first),
    0,
    inf_text_buffer_get_length(INF_TEXT_BUFFER(first))
  );
  second_chunk = inf_text_buffer_get_slice(
    INF_TEXT_BUFFER(second),
    0,
    inf_text_buffer_get_length(INF_TEXT_BUFFER(second))
  );

  result = inf_text_chunk_equal(first_chunk, second_chunk);

  inf_text_chunk_free(first_chunk);
  inf_text_chunk_free(second_chunk);
  g_object_unref(G_OBJECT(first));
  g_object_unref(G_OBJECT(second));

  return result;
}

static gboolean
test_c1(InfAdoptedOperation* op1,
        InfAdoptedOperation* op2,
        InfAdoptedUser* user1,
        InfAdoptedUser* user2,
        InfAdoptedConcurrencyId cid12)
{
  InfTextDefaultBuffer* first;
  InfTextDefaultBuffer* second;
  InfTextChunk* first_chunk;
  InfTextChunk* second_chunk;
  InfAdoptedOperation* transformed;
  int result;

  first = inf_text_default_buffer_new("UTF-8");

  inf_text_buffer_insert_text(
    INF_TEXT_BUFFER(first),
    0,
    EXAMPLE_DOCUMENT,
    strlen(EXAMPLE_DOCUMENT),
    strlen(EXAMPLE_DOCUMENT),
    NULL
  );

  second = inf_text_default_buffer_new("UTF-8");
  inf_text_buffer_insert_text(
    INF_TEXT_BUFFER(second),
    0,
    EXAMPLE_DOCUMENT,
    strlen(EXAMPLE_DOCUMENT),
    strlen(EXAMPLE_DOCUMENT),
    NULL
  );

  inf_adopted_operation_apply(op1, user1, INF_BUFFER(first));
  transformed = inf_adopted_operation_transform(op2, op1, op2, op1, -cid12);
  inf_adopted_operation_apply(transformed, user2, INF_BUFFER(first));
  g_object_unref(G_OBJECT(transformed));

  inf_adopted_operation_apply(op2, user2, INF_BUFFER(second));
  transformed = inf_adopted_operation_transform(op1, op2, op1, op2, cid12);
  inf_adopted_operation_apply(transformed, user1, INF_BUFFER(second));
  g_object_unref(G_OBJECT(transformed));

  first_chunk = inf_text_buffer_get_slice(
    INF_TEXT_BUFFER(first),
    0,
    inf_text_buffer_get_length(INF_TEXT_BUFFER(first))
  );
  second_chunk = inf_text_buffer_get_slice(
    INF_TEXT_BUFFER(second),
    0,
    inf_text_buffer_get_length(INF_TEXT_BUFFER(second))
  );

  result = inf_text_chunk_equal(first_chunk, second_chunk);

  inf_text_chunk_free(first_chunk);
  inf_text_chunk_free(second_chunk);
  g_object_unref(G_OBJECT(first));
  g_object_unref(G_OBJECT(second));

  return result;
}

static gboolean
test_c2(InfAdoptedOperation* op1,
        InfAdoptedOperation* op2,
        InfAdoptedOperation* op3,
        InfAdoptedConcurrencyId cid12,
        InfAdoptedConcurrencyId cid13,
        InfAdoptedConcurrencyId cid23,
        InfAdoptedUser* user3)
{
  InfAdoptedOperation* temp1;
  InfAdoptedOperation* temp2;
  InfAdoptedOperation* result1;
  InfAdoptedOperation* result2;
  InfTextDefaultBuffer* first;
  InfTextDefaultBuffer* second;
  InfTextChunk* first_chunk;
  InfTextChunk* second_chunk;
  gboolean retval;

  first = inf_text_default_buffer_new("UTF-8");

  inf_text_buffer_insert_text(
    INF_TEXT_BUFFER(first),
    0,
    EXAMPLE_DOCUMENT,
    strlen(EXAMPLE_DOCUMENT),
    strlen(EXAMPLE_DOCUMENT),
    NULL
  );

  second = inf_text_default_buffer_new("UTF-8");
  inf_text_buffer_insert_text(
    INF_TEXT_BUFFER(second),
    0,
    EXAMPLE_DOCUMENT,
    strlen(EXAMPLE_DOCUMENT),
    strlen(EXAMPLE_DOCUMENT),
    NULL
  );

  temp1 = inf_adopted_operation_transform(op2, op1, op2, op1, -cid12);
  temp2 = inf_adopted_operation_transform(op3, op1, op3, op1, -cid13);
  result1 = inf_adopted_operation_transform(temp2, temp1, op3, op2, -cid23);
  g_object_unref(G_OBJECT(temp1));
  g_object_unref(G_OBJECT(temp2));

  temp1 = inf_adopted_operation_transform(op1, op2, op1, op2, cid12);
  temp2 = inf_adopted_operation_transform(op3, op2, op3, op2, -cid23);
  result2 = inf_adopted_operation_transform(temp2, temp1, op3, op1, -cid13);
  g_object_unref(G_OBJECT(temp1));
  g_object_unref(G_OBJECT(temp2));

  inf_adopted_operation_apply(result1, user3, INF_BUFFER(first));
  inf_adopted_operation_apply(result2, user3, INF_BUFFER(second));

  first_chunk = inf_text_buffer_get_slice(
    INF_TEXT_BUFFER(first),
    0,
    inf_text_buffer_get_length(INF_TEXT_BUFFER(first))
  );
  second_chunk = inf_text_buffer_get_slice(
    INF_TEXT_BUFFER(second),
    0,
    inf_text_buffer_get_length(INF_TEXT_BUFFER(second))
  );

  retval = inf_text_chunk_equal(first_chunk, second_chunk);

  inf_text_chunk_free(first_chunk);
  inf_text_chunk_free(second_chunk);

  g_object_unref(G_OBJECT(result1));
  g_object_unref(G_OBJECT(result2));
  return retval;
}

static InfAdoptedConcurrencyId
cid(InfAdoptedOperation** first,
    InfAdoptedOperation** second)
{
  if(first > second)
    return INF_ADOPTED_CONCURRENCY_SELF;
  else if(first < second)
    return INF_ADOPTED_CONCURRENCY_OTHER;
  else
    g_assert_not_reached();
}

static void
perform_undo(InfAdoptedOperation** begin,
             InfAdoptedOperation** end,
             InfAdoptedUser** users,
             test_result* result)
{
  InfAdoptedOperation** _1;
  for(_1 = begin; _1 != end; ++ _1)
  {
    ++ result->total;
    if(test_undo(*_1, users[_1 - begin]))
      ++ result->passed;
  }
}

static void
perform_c1(InfAdoptedOperation** begin,
           InfAdoptedOperation** end,
           InfAdoptedUser** users,
           test_result* result)
{
  InfAdoptedOperation** _1;
  InfAdoptedOperation** _2;

  for(_1 = begin; _1 != end; ++ _1)
  {
    for(_2 = begin; _2 != end; ++ _2)
    {
      if(_1 != _2)
      {
        ++ result->total;
        if(test_c1(*_1, *_2, users[_1 - begin], users[_2 - begin], cid(_1, _2)))
          ++ result->passed;
      }
    }
  }
}

static void
perform_c2(InfAdoptedOperation** begin,
           InfAdoptedOperation** end,
           InfAdoptedUser** users,
           test_result* result)
{
  InfAdoptedOperation** _1;
  InfAdoptedOperation** _2;
  InfAdoptedOperation** _3;
  InfAdoptedConcurrencyId cid12;
  InfAdoptedConcurrencyId cid13;
  InfAdoptedConcurrencyId cid23;

  for(_1 = begin; _1 != end; ++ _1)
  {
    for(_2 = begin; _2 != end; ++ _2)
    {
      for(_3 = begin; _3 != end; ++ _3)
      {
        if(_1 != _2 && _1 != _3 && _2 != _3)
        {
          cid12 = cid(_1, _2);
          cid13 = cid(_1, _3);
          cid23 = cid(_2, _3);

          ++ result->total;
          if(test_c2(*_1, *_2, *_3, cid12, cid13, cid23, users[_3 - begin]))
            ++ result->passed;
        }
      }
    }
  }
}

int main()
{
  InfAdoptedOperation** operations;
  InfAdoptedUser** users;
  InfTextChunk* document;
  test_result result;
  guint i;
  int retval;

  g_type_init();

  retval = 0;

  operations = g_malloc(
    sizeof(InfAdoptedOperation*) * G_N_ELEMENTS(OPERATIONS)
  );
  
  users = g_malloc(sizeof(InfAdoptedUser*) * G_N_ELEMENTS(OPERATIONS));

  document = inf_text_chunk_new("UTF-8");
  inf_text_chunk_insert_text(
    document,
    0,
    EXAMPLE_DOCUMENT,
    strlen(EXAMPLE_DOCUMENT),
    strlen(EXAMPLE_DOCUMENT),
    0
  );

  for(i = 0; i < G_N_ELEMENTS(OPERATIONS); ++ i)
  {
    users[i] = INF_ADOPTED_USER(
      g_object_new(INF_TEXT_TYPE_USER, "id", i + 1, NULL)
    );

    operations[i] = def_to_operation(
      &OPERATIONS[i],
      document,
      users[i]
    );
  }

  inf_text_chunk_free(document);

  result.passed = 0;
  result.total = 0;

  perform_undo(
    operations,
    operations + G_N_ELEMENTS(OPERATIONS),
    users,
    &result
  );

  printf("UNDO: %u out of %u passed\n", result.passed, result.total);
  if(result.passed < result.total)
    retval = -1;

  result.passed = 0;
  result.total = 0;

  perform_c1(
    operations,
    operations + G_N_ELEMENTS(OPERATIONS),
    users,
    &result
  );

  printf("C1: %u out of %u passed\n", result.passed, result.total);
  if(result.passed < result.total)
    retval = -1;

  result.passed = 0;
  result.total = 0;

  perform_c2(
    operations,
    operations + G_N_ELEMENTS(OPERATIONS),
    users,
    &result
  );

  printf("C2: %u out of %u passed\n", result.passed, result.total);
  if(result.passed < result.total)
    retval = -1;

  for(i = 0; i < G_N_ELEMENTS(OPERATIONS); ++ i)
  {
    g_object_unref(G_OBJECT(operations[i]));
    g_object_unref(G_OBJECT(users[i]));
  }

  g_free(operations);
  g_free(users);

  return retval;
}
