---
title: Using Markdown and Github Actions to Publish a Static Blog
date: February 02, 2022
---

> First, I do not sit down at my desk to put into verse something that is 
> already clear in my mind. If it were clear in my mind, I should have no 
> incentive or need to write about it. We do not write in order to be 
> understood; we write in order to understand.
>
> \- C.S. Lewis

## What is this?

This is a static, Github-hosted, markdown-based, open-source blog, enabling 
a simple and content-focused writing process. My goal for this specific blog
is, as the eloquent C.S. Lewis explained, to provide myself with a personal 
outlet to write of my various programming projects or merely anything I think 
is worth writing about in an effort to further my understanding of the topic,
document the progress of these projects, and generally hone my skills as a
writer.

### Why not use an existing blogging platform?

To achieve this goal, I did not need (or want) any of the fancy features
provided by a blogging platform such as Medium or Wordpress. Furthermore,
markdown offers an extremely simple way to express every construct I would need
for a blog such as this. As a strong believer in the open-source process and 
community, I also wanted to provide a minimal and easy way for others to create
similar blogs.

## Why should you consider using this?

### Static

This blog has no moving parts, no forms, no searchbar, not even any JavaScript;
what you see is what you get. This allows both the writer and the reader, to
focus solely on the content of the blog. 

### Github-hosted

I am using Github Pages to serve the pages generated from the raw markdown
(more on that later). This means that using this pipeline is 100% free. 
Furthermore, you need not worry about self-hosting.

### Markdown-based

[This blogpost](https://github.com/CoBrooks/blog/blob/main/pages/00%20-%20Using%20Markdown%20and%20Github%20Actions%20to%20Publish%20a%20Static%20Blog.md) 
and the ones that will follow are written entirely in markdown. By using a 
tool called [pandoc](https://pandoc.org/), 
this markdown is converted to html, allowing me to quickly and easily create 
and generate tables, syntax-highlighted codeblocks, even diagrams and 
visualizations with [Mermaid](https://mermaid-js.github.io/mermaid/).

### Open-source

The [repo](https://github.com/CoBrooks/blog) hosting this blog is public, 
allowing anyone and everyone to fork or clone it to set up their own blog.
I have set up a Github Action that automatically generates a table of contents
and individual blogposts from the markdown files within the `pages/` directory
upon each push to the main branch. This also opens up a plethora of Github 
(and git) features to the writing and revision process. For example, if someone 
notices a typo or if something becomes outdated, one can submit a pull request 
to suggest a fix. Additionally, Github projects allows one to see planned 
blogposts and what writing phase said post is currently in.

## The meat and potatoes

### Project structure

If you take a look at the [repo](https://github.com/CoBrooks/blog), you should 
see four folders. Starting from the top, we have the `.github/workflows/`
directory, containing the YAML file that describes the pipeline for generating
the blog upon a `git push`. Next is the `pages/` directory, which contains all
of the blogposts in their markdown format. Following this is the `styling/` 
directory with houses the sole stylesheet for the blog. Lastly we have the
`templates/` directory which contains two html files: one for generating the
table of contents and the other for the individual blogposts. Both of these 
templates are used by pandoc during the publish job. 

### The publish job

There are four main steps contained in [publish.yml](https://github.com/CoBrooks/blog/blob/main/.github/workflows/publish.yml):

- `run: mkdir -p output/css` and `uses: r-lib/actions/setup-pandoc@master` 
create the output folder and download pandoc respectively. 
- `generate index.html` runs through all of the .md files in `pages/` and 
generates the table of contents.
- `generate html files` again iterates through each .md file in `pages/` and 
generates the final blogposts.
- `run: cp ./styling/style.css...` and `uses: JamesIves/github-pages-deploy-action@v4.2.3` finish off the process by copying the stylesheet to the output 
directory and push said output directory to the `gh-pages` branch respectively.

## Conclusion

I hope that this blogpost was informative and/or interesting. My goal going
forward is to write blogposts on a variety of subjects ranging from, 
as I mentioned previously, programming projects to literature or theology; 
anything I find interesting, really. The next one I have planned is one 
concerning the intriguing nature of the RDF data model.

