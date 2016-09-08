require('git')
require('fileutils')
require('open3')

# String String String String -> Git::Base
# Robustly clones a source repo to the given target
# repo_url: String   = URL to repository to clone from
# branch: String     = Commit SHA or Branch Name of a branch to switch to
# ref_commit: String = Commit SHA, Tag, or Branch Name to serve as the tag to
#                      branch off of
# local_path: String = File path to where the cloned repository should live on
#                      the local filesystem
def robust_clone(repo_url, local_path)
  g = nil
  if File.exist?(local_path)
    if File.exist?(File.join(local_path, '.git'))
      g = Git.open(local_path)
      r = g.remotes.select {|r| r.url == repo_url}[0]
      origin = g.remotes.select {|r| r.name == "origin"}[0]
      if r.nil? or r != origin
        g.remove_remote("origin") unless origin.nil?
        g.add_remote("origin", repo_url)
      end
    else
      if Dir[File.join(local_path, '*')].empty?
        g = Git.clone(repo_url, local_path)
      else
        raise "local_path \"#{local_path}\" exists but is NOT empty; please remove files and try again"
      end
    end
  else
    FileUtils.mkdir_p(local_path)
    g = Git.clone(repo_url, local_path)
  end
  g
end

def find_commit_from_substr(g, substr)
  c = nil
  g.tags.each do |t|
    if t.name.include?(substr)
      c = t.objectish
      break
    end
  end
  c
end

def determine_branch
  if ! ENV['SRC_BRANCH'].nil?
    ENV['SRC_BRANCH']
  elsif ! ENV['TRAVIS_BRANCH'].nil?
    puts("Tried to find environment variable SRC_BRANCH but it was nil...")
    puts("trying TRAVIS_BRANCH...")
    ENV['TRAVIS_BRANCH']
  elsif ! ENV['APPVEYOR_REPO_BRANCH'].nil?
    puts("Tried to find environment variable SRC_BRANCH but it was nil...")
    puts("trying APPVEYOR_REPO_BRANCH...")
    ENV['APPVEYOR_REPO_BRANCH']
  else
    puts("Could not determine branch... exiting with non-zero status")
    exit(1)
  end
end

def mimic_source(g, branch, src_commit)
  puts("Mimicing source repository")
  puts("- branch: #{branch}")
  puts("- src_commit: #{src_commit}")
  #ref_commit = find_commit_from_substr(g, src_commit)
  #puts("- ref_commit = #{ref_commit}")
  #if ref_commit.nil?
  #  puts("no relevant tag in repository")
  #elsif not g.is_branch?(branch)
  #  puts("checking out upstream")
  #  g.checkout(ref_commit)
  #end
  puts("creating and checking out new branch, #{branch}")
  g.branch(branch).checkout
end

def run_case(in_root, out_root, arch, a, c)
  path = File.join(out_root, arch, c[:id])
  FileUtils.mkdir_p(path) unless File.exist?(path)
  FileUtils.cp(File.join(in_root, a[:exe]), path)
  c[:input_files].each do |f|
    FileUtils.cp(File.join(in_root, f), path)
  end
  cmd = [
    "cd #{path}",
    "#{a[:cmd]} #{c[:args].join(' ')}"
  ].join(" && ")
  stdout, stderr, exitcode = Open3.capture3(cmd)
  files_to_add = c[:output_files].map {|o|
    File.join(path, File.basename(o))
  }
  w = lambda do |name, data|
    p = File.join(path, name)
    File.write(p, data)
    files_to_add << p
  end
  w['stdout.log', stdout]
  w['stderr.log', stderr]
  w['exitcode.log', exitcode]
  c[:input_files].each do |f|
    FileUtils.rm(File.join(path, File.basename(f)))
  end
  FileUtils.rm(File.join(path, File.basename(a[:exe])))
  files_to_add
end

# robust pull/push
def robust_push_pull(g, branch, the_commit)
  puts("Starting robust_pull_push!")
  begin
    puts("Attempting to pull")
    g.pull('origin', branch) if g.is_remote_branch?(branch)
    puts("Pull attempt succeeded")
  rescue => e
    puts("Trying to fix suspected auto-merge conflict")
    puts("Error: #{e.message}")
    puts("Git repo directory: #{g.dir}")
    # OK, we probably have a merge conflict
    # we need to:
    # 1. find which files are in conflict
    # 2. `git checkout --ours #{filename}` for each file in conflict
    # 3. re-add the files
    # 4. re-commit the files with some sort of commit message

    # `git diff --name-only --diff-filter=U`
    puts("Asking git for list of conflicted files")
    files_in_conflict = `cd #{g.dir} && git diff --name-only --diff-filter=U`
    puts("files in conflict: #{files_in_conflict.split.inspect}")
    files_in_conflict.split.each do |fname|
      puts("checking out and re-adding #{fname}")
      `cd #{g.dir} && git checkout --ours #{fname}`
      g.add(fname)
    end
    puts("(Re-)Committing...")
    g.commit("Commit to fix auto-merge conflict\n#{the_commit}")
    puts("Done!")
  end
  puts("Pushing to origin!")
  g.push('origin', branch, {:tags=>true})
  puts("Done robust_pull_push!")
end
