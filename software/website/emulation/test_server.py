from http.server import BaseHTTPRequestHandler, HTTPServer
from urllib import parse

MAX_LIST = 100

class RequestHandler(BaseHTTPRequestHandler):

    images = ['/logo.png', '/favicon.ico']
    data = ['/whitelist.txt', '/blacklist.txt', '/log.csv']
    get_paths = ['/homepage','/blacklist','/whitelist','/log','/settings']
    post_paths = ['/blacklist/add', '/blacklist/delete', '/whitelist/add', '/whitelist/delete']

    def do_GET(self):
        request = parse.urlparse(self.path)
        print(request.path) 
        if request.path in self.get_paths:
            self.send_response(200)
            self.send_header('Content-Type','text/html; charset=utf-8')
            self.end_headers()
            with open('application{}.html'.format(request.path), 'rb') as f:
                self.wfile.write(f.read())
        elif request.path in self.images:
            self.send_response(200)
            self.send_header('Content-Type','image/*')
            self.end_headers()
            with open(request.path[1:], 'rb') as f:
                self.wfile.write(f.read())
        elif request.path in self.data:
            self.send_response(200)
            self.send_header('Content-Type','text/*')
            self.end_headers()
            with open('data{}'.format(request.path), 'rb') as f:
                self.wfile.write(f.read())
        else:
            print("Sending 404")
            self.send_response(404)
            self.send_header('Content-Type','text/html; charset=utf-8')
            self.end_headers()


    def do_POST(self):
        request = parse.urlparse(self.path)
        print(request.path)
        if request.path in self.post_paths:

            command = request.path.split('/')[-2:]
            data = self.rfile.read(int(self.headers['Content-Length'])).decode()
            print("Data: " + data)

            if command[1] == 'add':
                with open('data/{}.txt'.format(command[0]), 'r+') as f:

                    if len(f.readlines()) > MAX_LIST:
                        print("Error adding {} to {}".format(data, command[0]))
                        self.send_response(500)
                        self.send_header('Content-Type','text/html; charset=utf-8')
                        self.end_headers()
                        self.wfile.write(b'List full, delete entries to make room')
                    else:
                        print('Added {} to {}'.format(data, command[0]))
                        f.write(data+'\n')
                        self.send_response(200)
                        self.wfile.write(b'Success')

            elif command[1] == 'delete':
                print('Removing {} from {}'.format(data, command[0]))
                with open('data/{}.txt'.format(command[0]), 'r+') as f:
                    lines = [l.strip('\n\r') for l in f.readlines()]
                    print(lines)

                    if data in lines:
                        lines.remove(data)
                        print(lines)
                        f.seek(0)
                        f.truncate(0)
                        '\n'.join(lines)
                        f.writelines(lines)
                        self.send_response(200)
                    else:
                        print("{} not in {}".format(data, command[0]))
                        self.send_response(400)
                        self.send_header('Content-Type','text/html; charset=utf-8')
                        self.end_headers()
                        self.wfile.write('{} not in list'.format(data).encode())
        else:
            self.send_response(404)

    def print_headers(self):
        parsed_path = parse.urlparse(self.path)
        message_parts = [
            'CLIENT VALUES:',
            'client_address={} ({})'.format(
                self.client_address,
                self.address_string()),
            'command={}'.format(self.command),
            'path={}'.format(self.path),
            'real path={}'.format(parsed_path.path),
            'query={}'.format(parsed_path.query),
            'request_version={}'.format(self.request_version),
            '',
            'SERVER VALUES:',
            'server_version={}'.format(self.server_version),
            'sys_version={}'.format(self.sys_version),
            'protocol_version={}'.format(self.protocol_version),
            '',
            'HEADERS RECEIVED:',
        ]
        for name, value in sorted(self.headers.items()):
            message_parts.append(
                '{}={}'.format(name, value.rstrip())
            )
        message_parts.append('')
        message = '\r\n'.join(message_parts)
        print(message)




if __name__ == '__main__':
    try:
        server = HTTPServer(('localhost',8000), RequestHandler)
        print("Starting Server...")
        server.serve_forever()
    except Exception:
        server.shutdown()