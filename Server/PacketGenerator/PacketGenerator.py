import argparse
import jinja2
import ProtoParser
import os

def main():

	arg_parser = argparse.ArgumentParser(description = 'PacketGenerator')
	arg_parser.add_argument('--path', type=str, default='', help='proto path')
	arg_parser.add_argument('--out_path', type=str, default='', help='proto path')
	arg_parser.add_argument('--output', type=str, default='TestPacketHandler', help='output file')
	arg_parser.add_argument('--process', type=str, default='S', help='')
	args = arg_parser.parse_args()

	parser = ProtoParser.ProtoParser(1000, args.process)
	parser.parse_proto(args.path)

	script_dir = os.path.dirname(os.path.abspath(__file__))
	template_dir = os.path.join(script_dir, "Templates")

	file_loader = jinja2.FileSystemLoader(template_dir)
	env = jinja2.Environment(loader=file_loader)

	template = env.get_template('PacketHandler.h')
	output = template.render(parser=parser, output=args.output)

	out_dir = os.path.join(args.out_path, args.output)
	f = open(out_dir+'.h', 'w+')
	f.write(output)
	f.close()

	print(output)
	return

if __name__ == '__main__':
	main()