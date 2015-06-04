require 'spec_helper'
require 'builder'
require 'nokogiri'
describe "RSolr::Xml" do
  
  let(:generator){ RSolr::Xml::Generator.new }

  builder_engines = { 
    :builder   => { :val => false, :class => Builder::XmlMarkup, :engine => Builder::XmlMarkup.new(:indent => 0, :margin => 0, :encoding => 'UTF-8') },
    :nokogiri  => { :val => true,  :class => Nokogiri::XML::Builder, :engine => Nokogiri::XML::Builder.new }
  }

  [:builder,:nokogiri].each do |engine_name|
    describe engine_name do
      before :all do
        @engine = builder_engines[engine_name]
        @old_ng_setting = RSolr::Xml::Generator.use_nokogiri
        RSolr::Xml::Generator.use_nokogiri = @engine[:val]
      end

      after :all do
        RSolr::Xml::Generator.use_nokogiri = @old_ng_setting
      end
  
      before :each do
        builder_engines.each_pair do |name,spec|
          expect(spec[:class]).not_to receive(:new) unless name == engine_name
        end
      end

      context :xml_engine do
        it "should use #{engine_name}" do
          expect(@engine[:class]).to receive(:new).and_return(@engine[:engine])
          generator.send(:commit)
        end
      end
  
      # call all of the simple methods...
      # make sure the xml string is valid
      # ensure the class is actually Solr::XML
      [:optimize, :rollback, :commit].each do |meth|
        it "#{meth} should generator xml" do
          result = generator.send(meth)
          expect(result).to eq("<?xml version=\"1.0\" encoding=\"UTF-8\"?><#{meth}/>")
        end
      end
  
      context :add do
    
        it 'should yield a Message::Document object when #add is called with a block' do
          documents = [{:id=>1, :name=>'sam', :cat=>['cat 1', 'cat 2']}]
          add_attrs = {:boost=>200.00}
          result = generator.add(documents, add_attrs) do |doc|
            doc.field_by_name(:name).attrs[:boost] = 10
            expect(doc.fields.size).to eq(4)
            expect(doc.fields_by_name(:cat).size).to eq(2)
          end
          expect(result).to match(%r(name="cat">cat 1</field>))
          expect(result).to match(%r(name="cat">cat 2</field>))
          expect(result).to match(%r(<add boost="200.0">))
          expect(result).to match(%r(boost="10"))
          expect(result).to match(%r(<field name="id">1</field>))
        end
    
        # add a single hash ("doc")
        it 'should create an add from a hash' do
          data = {
            :id=>1,
            :name=>'matt'
          }
          result = generator.add(data)
          expect(result).to match(/<field name="name">matt<\/field>/)
          expect(result).to match(/<field name="id">1<\/field>/)
        end

        # add an array of hashes
        it 'should create many adds from an array of hashes' do
          data = [
            {
              :id=>1,
              :name=>'matt'
            },
            {
              :id=>2,
              :name=>'sam'
            }
          ]
          message = generator.add(data)
          expected = "<?xml version=\"1.0\" encoding=\"UTF-8\"?><add><doc><field name=\"id\">1</field><field name=\"name\">matt</field></doc><doc><field name=\"id\">2</field><field name=\"name\">sam</field></doc></add>"
          expect(message).to match(/<field name="name">matt<\/field>/)
          expect(message).to match(/<field name="name">sam<\/field>/)
        end

        # multiValue field support test, thanks to Fouad Mardini!
        it 'should create multiple fields from array values' do
          data = {
            :id   => 1,
            :name => ['matt1', 'matt2']
          }
          result = generator.add(data)
          expect(result).to match(/<field name="name">matt1<\/field>/)
          expect(result).to match(/<field name="name">matt2<\/field>/)
        end

        it 'should create an add from a single Message::Document' do
          document = RSolr::Xml::Document.new
          document.add_field('id', 1)
          document.add_field('name', 'matt', :boost => 2.0)
          result = generator.add(document)
          expect(result).to match(Regexp.escape('<?xml version="1.0" encoding="UTF-8"?>'))
          expect(result).to match(/<field name="id">1<\/field>/)
          expect(result).to match Regexp.escape('boost="2.0"')
          expect(result).to match Regexp.escape('name="name"')
          expect(result).to match Regexp.escape('matt</field>')
        end
    
        it 'should create adds from multiple Message::Documents' do
          documents = (1..2).map do |i|
            doc = RSolr::Xml::Document.new
            doc.add_field('id', i)
            doc.add_field('name', "matt#{i}")
            doc
          end
          result = generator.add(documents)
          expect(result).to match(/<field name="name">matt1<\/field>/)
          expect(result).to match(/<field name="name">matt2<\/field>/)
        end
    
      end
  
      context :delete_by_id do
    
        it 'should create a doc id delete' do
          expect(generator.delete_by_id(10)).to eq("<?xml version=\"1.0\" encoding=\"UTF-8\"?><delete><id>10</id></delete>")
        end
    
        it 'should create many doc id deletes' do
          expect(generator.delete_by_id([1, 2, 3])).to eq("<?xml version=\"1.0\" encoding=\"UTF-8\"?><delete><id>1</id><id>2</id><id>3</id></delete>")
        end
    
      end
  
      context :delete_by_query do
        it 'should create a query delete' do
          expect(generator.delete_by_query('status:"LOST"')).to eq("<?xml version=\"1.0\" encoding=\"UTF-8\"?><delete><query>status:\"LOST\"</query></delete>")
        end
    
        it 'should create many query deletes' do
          expect(generator.delete_by_query(['status:"LOST"', 'quantity:0'])).to eq("<?xml version=\"1.0\" encoding=\"UTF-8\"?><delete><query>status:\"LOST\"</query><query>quantity:0</query></delete>")
        end
      end

    end
  end
    
end
