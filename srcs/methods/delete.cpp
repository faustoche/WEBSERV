#include "server.hpp"

/* Handles delete request by checking permissions, and removing specific files if allowed */

void	c_response::handle_delete_request(const c_request &request, const string &version, string file_path)
{
	if (file_path.find("..") != string::npos)
	{
		build_error_response(403, version, request);
		return ;
	}
	if (!is_existing_file(file_path))
	{
		build_error_response(404, version, request);
		return ;
	}

	struct stat file_stat;
	string dir = file_path.substr(0, file_path.find_last_of('/'));
	if (stat(file_path.c_str(), &file_stat) == 0 && S_ISDIR(file_stat.st_mode))
	{
		build_error_response(403, version, request);
		return ;
	}
	if (access(dir.c_str(), W_OK) != 0)
	{
		build_error_response(409, version, request);
		return ;
	}
	if (access(file_path.c_str(), W_OK) != 0)
	{
		build_error_response(403, version, request);
		return ;
	}
	if (remove(file_path.c_str()) != 0)
	{
		build_error_response(500, version, request);
		return ;
	}
}

/* LOads the page, insert all the taks and build success response */

void c_response::load_todo_page(const string &version, const c_request &request)
{
	string filename = "./www/data/todo.txt";
	string todo_html = load_file_content("./www/todo.html");
	ifstream infile(filename.c_str());
	string tasks_html;
	string line;

	while (getline(infile, line))
	{
		tasks_html += "<div class=\"todo-item\">";
		tasks_html += "<span class=\"task-text\">" + line + "</span>";
		tasks_html += "<button class=\"delete-btn\" onclick=\"deleteTask('" + line + "')\">DELETE</button>";
		tasks_html += "</div>\n";
	}
	infile.close();

	size_t pos = todo_html.find("{{TASKS_HTML}}");
	if (pos != string::npos)
		todo_html.replace(pos, strlen("{{TASKS_HTML}}"), tasks_html);
	_file_content = todo_html;
	build_success_response("todo.html", version, request);
}

/* Delete a specific task from the todo list and reload the updated page */

void c_response::handle_delete_todo(const c_request &request, const string &version)
{
	string target = request.get_target();
	string task_to_delete;
	size_t pos = target.find("?task=");
	if (pos != string::npos)
	{
		task_to_delete = target.substr(pos + 6);
		task_to_delete = url_decode(task_to_delete);
	}
	else
	{
		build_error_response(400, version, request);
		return ;
	}

	string filename = "./www/data/todo.txt";
	ifstream infile(filename.c_str());
	if (!infile.is_open())
	{
		build_error_response(500, version, request);
		return ;
	}
	
	vector<string> tasks;
	string line;
	int line_count = 0;
	bool found = false;

	while (getline(infile, line))
	{
		line_count++;
		if (line == task_to_delete)
			found = true;
		else
			tasks.push_back(line);
	}
	infile.close();
	if (!found)
		_server.log_message("[ERROR] Task cannot be found");
	
	ofstream outfile(filename.c_str(), ios::trunc);
	if (!outfile.is_open())
	{
		build_error_response(500, version, request);
		return ;
	}

	for (size_t i = 0; i < tasks.size(); i++)
		outfile << tasks[i] << endl;
	outfile.close();
	load_todo_page(version, request);
}


/* Handle file delete request after checking filename and access permissions */

void c_response::handle_delete_upload(const c_request &request, const string &version)
{
	string target = request.get_target();
	string file_to_delete;
	size_t pos = target.find("?file=");
	
	if (pos != string::npos)
	{
		file_to_delete = target.substr(pos + 6);
		file_to_delete = url_decode(file_to_delete);
	}
	else
	{
		build_error_response(400, version, request);
		return ;
	}

	string filename = "./www/upload/" + file_to_delete;

	if (file_to_delete.find("..") != string::npos)
	{
		build_error_response(403, version, request);
		return ;
	}
	
	if (remove(filename.c_str()) != 0)
	{
		_server.log_message("[ERROR] file not found or cannot be deleted: " + filename);
		build_error_response(404, version, request);
		return ;
	}
	
	_server.log_message("[INFO] ✅ FILE DELETED: " + filename);
	load_upload_page(version, request);
}
