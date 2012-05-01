# Copyright (C) 2009-2012  Kouhei Sutou <kou@clear-code.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License version 2.1 as published by the Free Software Foundation.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

class ExpressionBuilderTest < Test::Unit::TestCase
  include GroongaTestUtils

  setup :setup_database
  setup :setup_tables
  setup :setup_data

  def setup_tables
    Groonga::Schema.define do |schema|
      schema.create_table("Pets",
                          :type => :hash,
                          :key_type => "ShortText") do |table|
        table.short_text("name")
      end

      schema.create_table("Sections",
                          :type => :patricia_trie,
                          :key_type => "ShortText") do |table|
      end

      schema.create_table("Users",
                          :type => :hash,
                          :key_type => "ShortText") do |table|
        table.short_text("name")
        table.uint32("hp")
        table.reference("pet", "Pets")
        table.reference("section", "Sections")
      end

      schema.create_table("Bookmarks") do |table|
        table.reference("user", "Users")
        table.short_text("uri")
      end

      schema.change_table("Sections") do |table|
        table.index("Users.section")
      end
    end

    define_users_name_index

    @pets = Groonga["Pets"]
    @users = Groonga["Users"]
    @bookmarks = Groonga["Bookmarks"]
  end

  def define_users_name_index
    Groonga::Schema.define do |schema|
      schema.create_table("Terms",
                          :type => :patricia_trie,
                          :default_tokenizer => "TokenBigram",
                          :key_type => "ShortText") do |table|
        table.index("Users.name")
      end
    end
  end

  def setup_data
    @morita = @users.add("morita",
                         :name => "mori daijiro",
                         :hp => 100,
                         :section => "search/core")
    @gunyara_kun = @users.add("gunyara-kun",
                              :name => "Tasuku SUENAGA",
                              :hp => 150,
                              :section => "suggest/all")
    @yu = @users.add("yu",
                     :name => "Yutaro Shimamura",
                     :hp => 200,
                     :section => "search/all")

    @groonga = @bookmarks.add(:user => @morita, :uri => "http://groonga.org/")
    @ruby = @bookmarks.add(:user => @morita, :uri => "http://ruby-lang.org/")
    @nico_dict = @bookmarks.add(:user => @gunyara_kun,
                                :uri => "http://dic.nicovideo.jp/")
  end

  class EqualityTest < self
    def setup_tables
      Groonga::Schema.define do |schema|
        schema.create_table("Users",
                            :type => :hash,
                            :key_type => "ShortText") do |table|
          table.short_text("name")
        end
      end

      @users = Groonga["Users"]
    end

    def setup_data
      @users.add("morita",      :name => "mori daijiro")
      @users.add("gunyara-kun", :name => "Tasuku SUENAGA")
      @users.add("yu",          :name => "Yutaro Shimamura")
    end

    class EqualTest < self
      def test_equal_without_index
        result = @users.select do |record|
          record["name"] == "mori daijiro"
        end
        assert_equal(["morita"],
                     result.collect {|record| record.key.key})
      end

      def test_equal_with_index
        define_users_name_index
        result = @users.select do |record|
          record["name"] == "mori daijiro"
        end
        assert_equal(["morita"],
                     result.collect {|record| record.key.key})
      end
    end

    class NotEqualTest < self
      setup :only_ruby19

      def test_without_index
        result = @users.select do |record|
          record["name"] != "mori daijiro"
        end
        assert_equal(["gunyara-kun", "yu"],
                     result.collect {|record| record.key.key})
      end

      def test_with_index
        define_users_name_index
        result = @users.select do |record|
          record["name"] != "mori daijiro"
        end
        assert_equal(["gunyara-kun", "yu"],
                     result.collect {|record| record.key.key})
      end
    end
  end

  class RelationTest < self
    def setup_tables
      Groonga::Schema.define do |schema|
        schema.create_table("Users",
                            :type => :hash,
                            :key_type => "ShortText") do |table|
          table.uint32("hp")
        end
      end

      @users = Groonga["Users"]
    end

    def setup_data
      @users.add("morita",      :hp => 100)
      @users.add("gunyara-kun", :hp => 150)
      @users.add("yu",          :hp => 200)
    end

    def test_less
      result = @users.select do |record|
        record["hp"] < 150
      end
      assert_equal(["morita"], result.collect {|record| record.key.key})
    end

    def test_less_equal
      result = @users.select do |record|
        record["hp"] <= 150
      end
      assert_equal(["morita", "gunyara-kun"],
                   result.collect {|record| record.key.key})
    end

    def test_greater
      result = @users.select do |record|
        record["hp"] > 150
      end
      assert_equal(["yu"], result.collect {|record| record.key.key})
    end

    def test_greater_equal
      result = @users.select do |record|
        record["hp"] >= 150
      end
      assert_equal(["gunyara-kun", "yu"],
                   result.collect {|record| record.key.key})
    end
  end

  class LogicalTest < self
    def test_and
      result = @users.select do |record|
        (record["hp"] > 100) & (record["hp"] <= 200)
      end
      assert_equal(["gunyara-kun", "yu"],
                   result.collect {|record| record.key.key})
    end
  end

  def test_match
    result = @users.select do |record|
      record["name"] =~ "ro"
    end
    assert_equal(["morita", "yu"],
                 result.collect {|record| record.key.key})
  end

  def test_prefix_saerch
    result = @users.select do |record|
      record.section.prefix_search("search")
    end
    assert_equal(["morita", "yu"].sort,
                 result.collect {|record| record.key.key}.sort)
  end

  def test_suffix_search
    result = @users.select do |record|
      record.name.suffix_search("jiro")
    end
    assert_equal(["morita"].sort,
                 result.collect {|record| record.key.key}.sort)
  end

  def test_query_string
    result = @users.select("name:@ro")
    assert_equal(["morita", "yu"],
                 result.collect {|record| record.key.key})
  end

  def test_record
    result = @bookmarks.select do |record|
      record["user"] == @morita
    end
    assert_equal(["http://groonga.org/", "http://ruby-lang.org/"],
                 result.collect {|record| record.key["uri"]})
  end

  def test_record_id
    result = @bookmarks.select do |record|
      record["user"] == @morita.id
    end
    assert_equal(["http://groonga.org/", "http://ruby-lang.org/"],
                 result.collect {|record| record.key["uri"]})
  end

  def test_record_id_object
    morita = Object.new
    morita_singleton_class = (class << morita; self; end)
    morita_id = @morita.id
    morita_singleton_class.send(:define_method, :record_id) do
      morita_id
    end
    result = @bookmarks.select do |record|
      record["user"] == morita
    end
    assert_equal(["http://groonga.org/", "http://ruby-lang.org/"],
                 result.collect {|record| record.key["uri"]})
  end

  def test_nested_column
    result = @bookmarks.select do |record|
      record[".user.name"] == @morita["name"]
    end
    assert_equal(["http://groonga.org/", "http://ruby-lang.org/"],
                 result.collect {|record| record["uri"]})
  end

  def test_method_chain
    result = @bookmarks.select do |record|
      record.user.name == @morita["name"]
    end
    assert_equal(["http://groonga.org/", "http://ruby-lang.org/"],
                 result.collect {|record| record["uri"]})
  end

  def test_deep_method_chain
    @pets.add("bob", :name => "morita Bob")
    @morita["pet"] = "bob"

    result = @bookmarks.select do |record|
      record.user.pet.name == "morita Bob"
    end
    assert_equal(["http://groonga.org/", "http://ruby-lang.org/"],
                 result.collect {|record| record["uri"]})
  end

  def test_nil_match
    @users.select do |record|
      exception = ArgumentError.new("match word should not be nil: Users.name")
      assert_raise(exception) do
        record["name"] =~ nil
      end
      record["name"] == "dummy"
    end
  end

  def test_id
    result = @users.select do |record|
      record.id == 1
    end
    assert_equal(["morita"],
                 result.collect {|record| record.key.key})
  end

  def test_key
    result = @users.select do |record|
      record.key == "morita"
    end
    assert_equal(["morita"],
                 result.collect {|record| record.key.key})
  end

  def test_score
    result = @users.select do |record|
      (record.name =~ "o") | (record.hp >= 150)
    end
    result = result.select do |record|
      record.score > 1
    end
    assert_equal(["yu"],
                 result.collect {|record| record["_key"]})
  end

  def test_n_sub_records
    result = @users.select do |record|
      (record.name =~ "o") | (record.hp >= 150)
    end
    result = result.select do |record|
      record.n_sub_records > 1
    end
    assert_equal(["yu"],
                 result.collect {|record| record["_key"]})
  end

  def test_array_result
    result = @users.select do |record|
      conditions = []
      conditions << (record.hp > 100)
      conditions << (record.hp < 200)
      conditions
    end
    assert_equal(["gunyara-kun"],
                 result.collect {|record| record.key.key})
  end
end
