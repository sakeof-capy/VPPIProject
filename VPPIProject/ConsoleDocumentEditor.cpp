#include "ConsoleDocumentEditor.h"
#include "Utilities.h"
#include "CommandException.h"
#include "ConcreteFactories.h"
#include "InvalidCastException.h"
#include <type_traits>
#include <memory>
#include <iostream>

#define ATTACH_METHOD(method_name) \
std::bind(&ConsoleDocumentEditor::method_name, this, std::placeholders::_1)

#define MAKE_FACTORY(factory_type) \
std::make_shared<factory_type>()

const char* const ConsoleDocumentEditor::s_APP_BORDER		= "###############################################\n";
const char* const ConsoleDocumentEditor::s_DOC_TOP_BORDER	= "\n/---------------------------------------------\\\n";
const char* const ConsoleDocumentEditor::s_DOC_MID_BORDER	= "|---------------------------------------------|\n";
const char* const ConsoleDocumentEditor::s_DOC_BOT_BORDER	= "\\---------------------------------------------/\n\n";
const char* const ConsoleDocumentEditor::s_DEFAULT_FILENAME = "Document";

ConsoleDocumentEditor::ConsoleDocumentEditor() :
	doctypes_to_factories_{},
	commands_
	(
		{
			{"?", ATTACH_METHOD(print_help)},
			{"new_document", ATTACH_METHOD(new_document)},
			{"open_document", ATTACH_METHOD(open_document)},
			{"close_document", ATTACH_METHOD(close_document)},
			{"show_current", ATTACH_METHOD(show_current)},
		}
	),
	output_(std::cout),
	input_(std::cin),
	current_header_(nullptr),
	current_document_(nullptr),
	doc_creator_(nullptr)
{
}

auto ConsoleDocumentEditor::execute() const noexcept -> int
{
	main_loop();
	return 0;
}

auto ConsoleDocumentEditor::print_help(const iterable& params) const -> void
{
	command_params_check(0u, params.size());
	for (auto&& [action, _] : commands_)
	{
		output_ << "# <" << action << '>' << '\n';
	}
}

auto ConsoleDocumentEditor::new_document(const iterable& params) -> void
{
	if (auto s = params.size(); s > 2 || s == 0)
		throw CommandException("***Invalid number of params");
	text_type doc_name = params.size() > 1
		? params[1]
		: s_DEFAULT_FILENAME;
	//IDocHeaderFactory& f = *s_DOCUMENT_TYPES_TO_FACTORIES.at(params[0]);
	
}

auto ConsoleDocumentEditor::open_document(const iterable& params) -> void
{
	command_params_check(1, params.size());
	auto factory = get_factory(params[0]);
	set_document(factory);
}

auto ConsoleDocumentEditor::close_document(const iterable& params) -> void
{
	command_params_check(0u, params.size());
	current_document_ = nullptr;
	current_header_ = nullptr;
}

auto ConsoleDocumentEditor::show_current(const iterable& params) const -> void
{
	command_params_check(0u, params.size());
	if (current_document_ == nullptr)
		throw CommandException("***No document open.");
	show_document(*current_document_, *current_header_);
}

auto ConsoleDocumentEditor::main_loop() const noexcept -> void
{
	text_type in_txt;
	output_ << s_APP_BORDER;
	output_ << "Welcome to our Document Editor\n";
	output_ << "What you can do [<command>]:\n";
	print_help({});
	output_ << "Awaiting your command...\n";
	do
	{
		output_ << '>' << ' ';
		std::getline(input_, in_txt);
		if (in_txt.empty()) continue;
		auto [first, params] = parse_user_command(in_txt);
		try
		{
			commands_.at(first).operator()(params);
		}
		catch (const CommandException& ex)
		{
			output_ << ex.what() << '\n';
		}
		catch (const std::out_of_range&)
		{
			output_ << "***No such a command.\n";
		}
	} while (true);
	output_ << "Have a nice day!\n";
	output_ << s_APP_BORDER;
}

auto ConsoleDocumentEditor::parse_user_command(const text_type& input)
const noexcept -> const std::pair<text_type, iterable>
{
	auto delims = " ";
	auto tokens = tokenize(input, text_type(delims));
	auto itor = tokens.begin();
	auto etor = tokens.end();
	auto first = *itor++;
	return { first, {itor, etor} };
}

auto ConsoleDocumentEditor::get_factory(const text_type& param) const -> ptr<factory_type>
{
	try
	{
		return doctypes_to_factories_.at(param);
	}
	catch (const std::out_of_range&)
	{
		throw CommandException("***Invalid number of params");
	}
}

auto ConsoleDocumentEditor::command_params_check
(
	const unsigned short needed_num_of_args, 
	const unsigned short actual_num_of_args
) const -> void
{
	if (needed_num_of_args == actual_num_of_args) return;
	auto msg = std::string("***Invalid number of params. Expected: ") +
		std::to_string(needed_num_of_args);
	throw CommandException(msg.c_str());
}

auto ConsoleDocumentEditor::show_document(const IDocument& document, const IHeader& header) const noexcept -> void
{
	auto& output = output_;
	auto print = [&output](const text_type& txt) { output << txt; };

	print(s_DOC_TOP_BORDER);
	{
		print(header.get_title() + '\n');
	}
	print(s_DOC_MID_BORDER);
	{
		print(document.get_text() + '\n');
	}
	print(s_DOC_BOT_BORDER);
}

auto ConsoleDocumentEditor::set_document(ptr<factory_type> factory) noexcept -> void
{
	using std::move;
	decltype(auto) header = factory->Create<IHeader>();
	decltype(auto) document = factory->Create<IDocument>();
	show_document(*document, *header);
	current_document_ = move(document);
	current_header_ = move(header);
}

auto ConsoleDocumentEditor::doctype_to_string(const DocumentType doc) -> text_type
{
	switch (doc)
	{
	case DocumentType::PLAIN_TEXT:
		return "plain_text";
	case DocumentType::MATH_TEXT:
		return "math_text";
	default:
		throw InvalidCastException("***No such a document type.");
	}
}
